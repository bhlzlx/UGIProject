#include <cstdio>
#include "json.hpp"
#include "ShaderCompilerUtility.h"
#include <regex>
#include <string>
#include <cassert>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <set>

std::array<std::vector<uint32_t>, (uint32_t)ugi::ShaderModuleType::ComputeShader+1 >  SPIRV_DATA;

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

// ==========================================================================
//  slangc 编译辅助
// ==========================================================================

// 从 .slang 源码中提取所有 import 语句引用的模块名
//   import fgui_lib;     →  "fgui_lib"
//   import fx_lib;        →  "fx_lib"
//   import a.b.c;         →  "a/b/c"        (路径分隔符转为 /)
static std::vector<std::string> ExtractSlangImports(const char* source) {
    std::vector<std::string> modules;
    std::string src(source);
    std::regex importRe(R"(import\s+([a-zA-Z_][\w.]*)\s*;)");
    auto begin = std::sregex_iterator(src.begin(), src.end(), importRe);
    auto end   = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        std::string raw = (*it)[1].str();
        // 点号 → 路径分隔符
        std::replace(raw.begin(), raw.end(), '.', '/');
        modules.push_back(raw);
    }
    return modules;
}

// 执行 slangc, 返回 spv 二进制 (uint32_t words)
//   args: 额外参数 (如 -target spirv -entry main)
static bool RunSlangC(const std::string& sourceFile,
                      const std::vector<std::string>& includePaths,
                      const std::string& entryPoint,
                      const std::string& profile,
                      std::vector<uint32_t>& outSpv)
{
    std::ostringstream cmd;

    // 基础命令
    cmd << "slangc \"" << sourceFile << "\"";

    // include 路径
    for (auto& p : includePaths) {
        cmd << " -I \"" << p << "\"";
    }

    // target + profile + capability
    cmd << " -target spirv";
    if (!profile.empty()) {
        cmd << " -profile " << profile;
    }
    cmd << " -capability vulkan_1_1";   // SV_VertexID 需要 DrawParameters

    // entry point
    cmd << " -entry " << entryPoint;

    // 输出到临时文件
    std::string tmpSpv = sourceFile + ".tmp.spv";
    cmd << " -o \"" << tmpSpv << "\"";

    // stderr 重定向
    cmd << " 2>&1";

    printf("  [slangc] %s\n", cmd.str().c_str());

    // 执行
    int ret = system(cmd.str().c_str());
    if (ret != 0) {
        printf("  [slangc] FAILED (exit %d)\n", ret);
        return false;
    }

    // 读取编译产物
    std::ifstream spvFile(tmpSpv, std::ios::binary | std::ios::ate);
    if (!spvFile) {
        printf("  [slangc] cannot open output: %s\n", tmpSpv.c_str());
        return false;
    }
    size_t fileSize = spvFile.tellg();
    spvFile.seekg(0, std::ios::beg);
    outSpv.resize(fileSize / sizeof(uint32_t));
    spvFile.read(reinterpret_cast<char*>(outSpv.data()), fileSize);

    // 清理临时文件
    std::remove(tmpSpv.c_str());

    printf("  [slangc] OK  (%zu bytes)\n", fileSize);
    return true;
}

// Slang 编译入口: 自动发现依赖 → 先编库 → 再编主 shader
static bool CompileSlangShader(const std::string& mainFile,
                               const std::vector<std::string>& importPaths,
                               const std::string& entryPoint,
                               const std::string& profile,
                               std::vector<uint32_t>& outSpv)
{
    // 1. 读主文件源码, 提取 import 依赖
    std::ifstream srcFile(mainFile);
    if (!srcFile) {
        printf("  [slangc] cannot read source: %s\n", mainFile.c_str());
        return false;
    }
    std::string src((std::istreambuf_iterator<char>(srcFile)),
                     std::istreambuf_iterator<char>());

    auto importedModules = ExtractSlangImports(src.c_str());
    printf("  [slangc] imports: %zu modules\n", importedModules.size());

    // 2. 编译每个依赖库 → .slang-module
    //    (先搜索 importPaths 找到 .slang 文件)
    std::vector<std::string> moduleFiles;
    std::string mainDir = mainFile.substr(0, mainFile.find_last_of("/\\") + 1);
    for (auto& modName : importedModules) {
        std::string found;
        // 先搜 mainFile 所在目录, 再搜 importPaths
        for (auto& ip : importPaths) {
            std::string searchPath = ip;
            if (searchPath.empty() || searchPath == ".") searchPath = mainDir;
            else if (searchPath[0] != '/' && searchPath.find(':') == std::string::npos) {
                // 相对路径 → 相对 mainFile 目录
                searchPath = mainDir + searchPath;
            }
            std::string candidate = searchPath + "/" + modName + ".slang";
            // normalize
            for (auto& ch : candidate) if (ch == '\\') ch = '/';

            std::ifstream test(candidate);
            if (test) {
                found = candidate;
                break;
            }
        }
        if (!found.empty()) {
            // 模块编译命令
            std::ostringstream cmd;
            cmd << "slangc \"" << found << "\" -target none -o \"" << found << ".slang-module\"";
            for (auto& p : importPaths) cmd << " -I \"" << p << "\"";
            cmd << " 2>&1";
            printf("  [slangc:lib] %s\n", cmd.str().c_str());
            int ret = system(cmd.str().c_str());
            if (ret != 0) {
                printf("  [slangc:lib] FAILED for %s\n", modName.c_str());
                // 非致命 — 主编译 import + -I 仍可回退
            } else {
                moduleFiles.push_back(found + ".slang-module");
            }
        } else {
            printf("  [slangc:lib] module '%s' not found in importPaths — will resolve from source\n",
                   modName.c_str());
        }
    }

    // 3. 用 -r 链接预编译模块 (如果有的话)
    //    如果某个模块没找到, slangc 会从 import + -I 回退
    std::ostringstream cmd;
    cmd << "slangc \"" << mainFile << "\"";
    for (auto& p : importPaths) cmd << " -I \"" << p << "\"";
    cmd << " -target spirv";
    if (!profile.empty()) cmd << " -profile " << profile;
    cmd << " -entry " << entryPoint;
    for (auto& mf : moduleFiles) {
        cmd << " -r \"" << mf << "\"";
    }
    std::string tmpSpv = mainFile + ".tmp.spv";
    cmd << " -o \"" << tmpSpv << "\" 2>&1";

    printf("  [slangc] %s\n", cmd.str().c_str());
    int ret = system(cmd.str().c_str());
    if (ret != 0) {
        printf("  [slangc] FAILED (exit %d)\n", ret);
        return false;
    }

    // 4. 读产物
    std::ifstream spvFile(tmpSpv, std::ios::binary | std::ios::ate);
    if (!spvFile) {
        printf("  [slangc] cannot open output: %s\n", tmpSpv.c_str());
        return false;
    }
    size_t fileSize = spvFile.tellg();
    spvFile.seekg(0, std::ios::beg);
    outSpv.resize(fileSize / sizeof(uint32_t));
    spvFile.read(reinterpret_cast<char*>(outSpv.data()), fileSize);
    std::remove(tmpSpv.c_str());

    printf("  [slangc] OK  (%zu bytes)\n", fileSize);
    return true;
}

// ==========================================================================
//  SPIRV-Reflect 反射 — 输入 SPIR-V 二进制，输出 descriptor/vertex/pushConst
// ==========================================================================
static void ReflectSpirV(const std::vector<uint32_t>& spv,
                         ugi::ShaderModuleType shaderModule,
                         ugi::PipelineDescription& desc,
                         bool slotOccupied[ugi::MaxArgumentCount][ugi::MaxDescriptorCount])
{
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(
        spv.size() * sizeof(uint32_t), spv.data(), &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        printf("  [reflect] SPIRV-Reflect parse failed: %d\n", result);
        return;
    }

    // 枚举 descriptor bindings
    uint32_t bindCount = 0;
    spvReflectEnumerateDescriptorBindings(&module, &bindCount, nullptr);
    std::vector<SpvReflectDescriptorBinding*> bindings(bindCount);
    spvReflectEnumerateDescriptorBindings(&module, &bindCount, bindings.data());

    // 枚举输入变量
    uint32_t inCount = 0;
    spvReflectEnumerateInputVariables(&module, &inCount, nullptr);
    std::vector<SpvReflectInterfaceVariable*> inputs(inCount);
    spvReflectEnumerateInputVariables(&module, &inCount, inputs.data());

    // 枚举 push constants
    uint32_t pcCount = 0;
    spvReflectEnumeratePushConstantBlocks(&module, &pcCount, nullptr);
    std::vector<SpvReflectBlockVariable*> pushConstants(pcCount);
    spvReflectEnumeratePushConstantBlocks(&module, &pcCount, pushConstants.data());

    const char* stageNames[] = {"VS","TCS","TES","GS","FS","CS"};
    printf("  [reflect] stage=%s  bindings=%u  inputs=%u  pushConst=%u\n",
        stageNames[(int)shaderModule], bindCount, inCount, pcCount);

    auto safeCopy = [](char* dst, size_t dstSize, const char* src) {
        std::string s(src);
        const char* prefix = "SLANG_ParameterGroup_";
        if (s.compare(0, strlen(prefix), prefix) == 0) {
            s = s.substr(strlen(prefix));
        }
        size_t pos = s.rfind("_std");
        if (pos != std::string::npos) {
            s = s.substr(0, pos);
        }
        size_t len = s.size();
        if (len >= dstSize) len = dstSize - 1;
        memcpy(dst, s.c_str(), len);
        dst[len] = '\0';
        if (s != src) {
            printf("  [normalize] '%s' → '%s'\n", src, dst);
        }
    };

    // vertex inputs
    if (shaderModule == ugi::ShaderModuleType::VertexShader) {
        for (auto* iv : inputs) {
            if (!iv || iv->location >= ugi::MaxVertexAttribute) continue;
            strcpy(desc.vertexLayout.buffers[iv->location].name, iv->name ? iv->name : "");
            desc.vertexLayout.buffers[iv->location].type = getVertexType(*iv);
            ++desc.vertexLayout.bufferCount;
        }
    }

    // push constants
    if (!pushConstants.empty()) {
        auto* pc = pushConstants[0];
        desc.pipelineConstants[(uint32_t)shaderModule].offset = (uint8_t)pc->offset;
        desc.pipelineConstants[(uint32_t)shaderModule].size  = (uint8_t)pc->size;
    }

    // descriptor bindings — 统一处理所有类型
    for (auto* b : bindings) {
        if (!b || b->set >= ugi::MaxArgumentCount || b->binding >= ugi::MaxDescriptorCount) continue;

        auto& d = desc.argumentLayouts[b->set].descriptors[b->binding];
        d.binding     = (uint8_t)b->binding;
        d.stageMask = (1u << (uint32_t)shaderModule); // Vk bit: VS=0x01, FS=0x10
        d.dataSize    = b->block.size;

        const char* name = b->name ? b->name : "";
        safeCopy(d.name, ugi::MaxNameLength, name);

        switch (b->descriptor_type) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            d.type = ugi::ArgumentDescriptorType::UniformBuffer;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            d.type = ugi::ArgumentDescriptorType::StorageBuffer;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            d.type = ugi::ArgumentDescriptorType::Sampler;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            d.type = ugi::ArgumentDescriptorType::Image;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            d.type = ugi::ArgumentDescriptorType::StorageImage;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            d.type = ugi::ArgumentDescriptorType::InputAttachment;
            break;
        default:
            continue;
        }

        if (!slotOccupied[b->set][b->binding]) {
            slotOccupied[b->set][b->binding] = true;
            ++desc.argumentLayouts[b->set].descriptorCount;
        }
        printf("  [reflect] set=%u bind=%u type=%d name='%s' size=%u\n",
            b->set, b->binding, b->descriptor_type, d.name, b->block.size);
    }

    spvReflectDestroyShaderModule(&module);
}

// 检测文件是否为 Slang
static bool IsSlangFile(const std::string& path) {
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = path.substr(dot);
    return ext == ".slang";
}

// 获取文件的基本名 (去掉扩展名)
static std::string GetBaseName(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    size_t dot   = path.rfind('.');
    if (slash == std::string::npos) slash = 0; else ++slash;
    if (dot == std::string::npos || dot < slash) dot = path.size();
    return path.substr(slash, dot - slash);
}

// vertex 入口点: 从文件名推断 (fx_main_a → fragmentMainA, main → main)
static std::string InferShaderEntry(const std::string& shaderPath, ugi::ShaderModuleType stage) {
    // 对 fragment shader, 入口可能是 "fragmentMain" 或 "main"
    // 目前规则: 如果文件中有 import, 入口为 "main" (fx_main_*.slang 的场景)
    //          否则默认为 "main"
    return "main";
}

#define VERTEX_MAP_ITEM( item ) { #item, ugi::VertexType::item },
std::map< std::string, ugi::VertexType > VertexTypeMapping = {
    VERTEX_MAP_ITEM(Float)
    VERTEX_MAP_ITEM(Float2)
    VERTEX_MAP_ITEM(Float3)
    VERTEX_MAP_ITEM(Float4)
    VERTEX_MAP_ITEM(Uint)
    VERTEX_MAP_ITEM(Uint2)
    VERTEX_MAP_ITEM(Uint3)
    VERTEX_MAP_ITEM(Uint4)
    VERTEX_MAP_ITEM(Half)
    VERTEX_MAP_ITEM(Half2)
    VERTEX_MAP_ITEM(Half3)
    VERTEX_MAP_ITEM(Half4)
    VERTEX_MAP_ITEM(UByte4)
    VERTEX_MAP_ITEM(UByte4N)
};

// ==========================================================================
//  main
// ==========================================================================

int main( int argc, char** argv ) {

    ugi::PipelineDescription pipelineDescription;
    if(!argv[1]) {
        return -1;
    }
    std::string assetRoot = argv[1];
    assetRoot.append("/");

    std::string configPath = assetRoot + "pipeline.json";
    nlohmann::json js;
    {
        FILE* jf = fopen(configPath.c_str(), "rb");
        if (!jf) {
            printf("%s not found!\n", configPath.c_str());
            return -1;
        }
        fseek(jf, 0, SEEK_END);
        size_t jsz = ftell(jf);
        fseek(jf, 0, SEEK_SET);
        std::vector<char> jsonBuffer(jsz + 1);
        fread(jsonBuffer.data(), 1, jsz, jf);
        fclose(jf);
        jsonBuffer[jsz] = 0;
        js = nlohmann::json::parse(jsonBuffer.data());
    }

    // ---- 读取 importPath (Slang) ----
    std::vector<std::string> importPaths;
    if (js.contains("importPath") && js["importPath"].is_array()) {
        for (auto& p : js["importPath"]) {
            if (p.is_string()) {
                importPaths.push_back(p.get<std::string>());
            }
        }
    }
    // 默认包含 shader 目录本身
    importPaths.push_back(assetRoot);

    // ---- 读取 profile (可选, 默认 spv_1_3 = SPIR-V 1.3) ----
    std::string slangProfile = "glsl_460";
    if (js.contains("slangProfile") && js["slangProfile"].is_string()) {
        slangProfile = js["slangProfile"].get<std::string>();
    }

    std::set<uint32_t> slangCompiledStages;  // 记录哪些 stage 走的是 Slang 路径

    std::string shaderFile;

    auto shaderModule = js["shaderModule"];
    if( shaderModule.is_null() ) {
        printf("shader module was empty!\n");
        return -1;
    } else {

        const std::map< const char*, ugi::ShaderModuleType > stageNameToType = {
            {"vert", ugi::ShaderModuleType::VertexShader},
            {"tesc", ugi::ShaderModuleType::TessellationControlShader},
            {"tese", ugi::ShaderModuleType::TessellationEvaluationShader},
            {"geom", ugi::ShaderModuleType::GeometryShader},
            {"frag", ugi::ShaderModuleType::FragmentShader},
            {"comp", ugi::ShaderModuleType::ComputeShader},
        };

        for( auto& stageEntry : stageNameToType ) {
            const char* stageName = stageEntry.first;
            ugi::ShaderModuleType shaderModuleType = stageEntry.second;

            if( shaderModule[stageName].is_string()) {
                shaderFile = shaderModule[stageName];

                printf("--- %s: %s ---\n", stageName, shaderFile.c_str());

                if (IsSlangFile(shaderFile)) {
                    // ===== Slang 路径 =====
                    std::string entryPoint = "main";
                    // 读 entryOverride (可选)
                    if (js.contains("entry") && js[stageName].is_string()) {
                        entryPoint = js[stageName].get<std::string>();
                    }

                    std::string fullPath = assetRoot + shaderFile;
                    std::string shaderDir = fullPath.substr(0, fullPath.find_last_of("/\\") + 1);
                    std::vector<std::string> allImportPaths = importPaths;
                    allImportPaths.push_back(shaderDir);

                    std::vector<uint32_t> compiledSPV;
                    bool ok = CompileSlangShader(fullPath, allImportPaths, entryPoint, slangProfile, compiledSPV);
                    if (!ok) {
                        printf("[FAIL] Slang compile: %s\n", shaderFile.c_str());
                        return -1;
                    }

                    // 存到全局 SPIRV_DATA, 标记为 Slang 编译
                    SPIRV_DATA[(uint32_t)shaderModuleType] = std::move(compiledSPV);
                    slangCompiledStages.insert((uint32_t)shaderModuleType);

                }
            }
        }
    }

    // ---- SPIRV-Reflect 反射 (所有编译的 stage) ----
    // 跨 stage 共享 binding 的去重 + stage 合并
    bool slotOccupied[ugi::MaxArgumentCount][ugi::MaxDescriptorCount] = {};
    uint8_t stageMask[ugi::MaxArgumentCount][ugi::MaxDescriptorCount] = {};

    for (auto idx : slangCompiledStages) {
        pipelineDescription.shaders[idx].type = (ugi::ShaderModuleType)idx;
        ReflectSpirV(SPIRV_DATA[idx], (ugi::ShaderModuleType)idx, pipelineDescription, slotOccupied);
        // 收集 stage mask: 哪些 stage 用了哪些 (set,binding)
        {
            SpvReflectShaderModule m;
            if (spvReflectCreateShaderModule(SPIRV_DATA[idx].size() * sizeof(uint32_t),
                    SPIRV_DATA[idx].data(), &m) == SPV_REFLECT_RESULT_SUCCESS) {
                uint32_t cnt = 0;
                spvReflectEnumerateDescriptorBindings(&m, &cnt, nullptr);
                std::vector<SpvReflectDescriptorBinding*> bnd(cnt);
                spvReflectEnumerateDescriptorBindings(&m, &cnt, bnd.data());
                for (auto* b : bnd) {
                    if (b && b->set < ugi::MaxArgumentCount && b->binding < ugi::MaxDescriptorCount)
                        stageMask[b->set][b->binding] |= (1u << (uint32_t)idx);
                }
                spvReflectDestroyShaderModule(&m);
            }
        }
    }

    // 对共享 binding 合并 shaderStage 为 VkShaderStageFlags 兼容 bitmask
    auto toVkStageBit = [](ugi::ShaderModuleType s) -> uint8_t {
        const uint8_t bits[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20 }; // VS..CS
        return bits[(int)s];
    };
    for (uint32_t i = 0; i < ugi::MaxArgumentCount; ++i) {
        for (uint32_t j = 0; j < ugi::MaxDescriptorCount; ++j) {
            auto& d = pipelineDescription.argumentLayouts[i].descriptors[j];
            if (d.binding != 0xff && stageMask[i][j]) {
                // 检查是否被多个 stage 使用
                int stageCount = 0;
                for (int s = 0; s < 6; ++s)
                    if (stageMask[i][j] & (1u << s)) ++stageCount;
                if (stageCount > 1) {
                    // 多 stage 共享 → 存 Vk bitmask
                    uint8_t combined = 0;
                    for (int s = 0; s < 6; ++s)
                        if (stageMask[i][j] & (1u << s)) combined |= toVkStageBit((ugi::ShaderModuleType)s);
                    d.stageMask = combined;
                    printf("  [stage] binding(%u,%u) shared by %d stages → 0x%02x\n", i, j, stageCount, combined);
                }
                // 单 stage → 保持原 enum 值不变
            }
        }
    }

    // ---- 汇总: 打印最终 argumentLayouts ----
    printf("=== Final argumentLayouts ===\n");
    for (uint32_t i = 0; i < ugi::MaxArgumentCount; ++i) {
        auto& layout = pipelineDescription.argumentLayouts[i];
        if (layout.descriptorCount) {
            printf("  set=%u  descriptorCount=%u\n", i, (unsigned)layout.descriptorCount);
            for (uint32_t j = 0; j < ugi::MaxDescriptorCount; ++j) {
                auto& d = layout.descriptors[j];
                if (d.binding != 0xff) {
                    const char* typeNames[] = {"Sampler","Image","StorageImage","StorageBuffer","UniformBuffer","UniformTexelBuffer","InputAttachment"};
                    printf("    binding=%u  type=%s  name='%s'  dataSize=%u\n",
                        (unsigned)d.binding,
                        (unsigned)d.type < 7 ? typeNames[(unsigned)d.type] : "???",
                        d.name, (unsigned)d.dataSize);
                }
            }
        }
    }
    printf("  argumentCount=%u\n", (unsigned)pipelineDescription.argumentCount);

    // ---- 计算 SPIR-V 在 pipeline.bin 中的偏移 ----
    uint32_t offsetCounter = sizeof(ugi::PipelineDescription);
    for( uint32_t i = 0; i<=(uint32_t)ugi::ShaderModuleType::ComputeShader; ++i) {
        if( SPIRV_DATA[i].size()) {
            pipelineDescription.shaders[i].spirvData = offsetCounter;
            pipelineDescription.shaders[i].spirvSize = (uint32_t)(SPIRV_DATA[i].size() * sizeof(uint32_t));
            offsetCounter += (uint32_t)(SPIRV_DATA[i].size() * sizeof(uint32_t));
        }
    }

    for( uint32_t i = 0; i<ugi::MaxArgumentCount; ++i) {
        if(pipelineDescription.argumentLayouts[i].descriptorCount) {
            ++pipelineDescription.argumentCount;
        }
    }

    // argument 排序
    for( uint32_t i = 0; i<ugi::MaxArgumentCount; ++i) {
        pipelineDescription.argumentLayouts[i].index = i;
    }
    std::sort( pipelineDescription.argumentLayouts, pipelineDescription.argumentLayouts+ugi::MaxArgumentCount, []( const ugi::ArgumentInfo& a, const ugi::ArgumentInfo& b ) {
        uint32_t avalue = (0x1 << a.index);
        if(!a.descriptorCount) avalue |= 0x10000000;
        uint32_t bvalue = (0x1 << b.index);
        if(!b.descriptorCount) bvalue |= 0x10000000;
        return avalue < bvalue;
    });

    // descriptor 排序
    for( uint32_t i = 0; i<ugi::MaxArgumentCount; ++i) {
        if(pipelineDescription.argumentLayouts[i].descriptorCount) {
            std::sort(
            pipelineDescription.argumentLayouts[i].descriptors,
            pipelineDescription.argumentLayouts[i].descriptors+ugi::MaxDescriptorCount,
            []( const ugi::ArgumentDescriptorInfo& a, const ugi::ArgumentDescriptorInfo& b) ->bool {
                return a.binding < b.binding;
            });
        }
    }

    // vertex layout
    uint32_t attrOffset = 0;
    for( uint32_t i = 0; i<pipelineDescription.vertexLayout.bufferCount; ++i) {
        pipelineDescription.vertexLayout.buffers[i].stride = getVertexSize(pipelineDescription.vertexLayout.buffers[i].type);
        pipelineDescription.vertexLayout.buffers[i].offset = 0;
        pipelineDescription.vertexLayout.buffers[i].instanceMode = 0;
    }

    auto combindVertexNode = js["combineVertex"];
    if( combindVertexNode.is_boolean()) {
        bool r = combindVertexNode;
        if(r) {
            uint32_t stride = 0;
            for( size_t i = 0; i<pipelineDescription.vertexLayout.bufferCount; ++i) {
                pipelineDescription.vertexLayout.buffers[i].offset = stride;
                stride += pipelineDescription.vertexLayout.buffers[i].stride;
            }
            for( size_t i = 0; i<pipelineDescription.vertexLayout.bufferCount; ++i) {
                pipelineDescription.vertexLayout.buffers[i].stride = stride;
            }
        }
    }

    std::string outputPath = assetRoot;
    if (outputPath.back() != '/') outputPath += '/';
    outputPath += "pipeline.bin";
    FILE* f = fopen(outputPath.c_str(), "wb");
    if (!f) {
        printf("[ERROR] cannot create output file: %s\n", outputPath.c_str());
        return -1;
    }
    fwrite(&pipelineDescription, sizeof(pipelineDescription), 1, f);
    for (auto& spv : SPIRV_DATA) {
        if (spv.size()) {
            fwrite(spv.data(), sizeof(uint32_t), spv.size(), f);
        }
    }
    fclose(f);

    // ---- 生成 C++ 头文件 ----
    auto& desc = pipelineDescription;

    // helper: 从 SPIRV-Reflect type_description 推断 C++ 类型名
    auto spvTypeToStr = [](const SpvReflectTypeDescription* td) -> std::string {
        if (!td) return "uint8_t";
        uint32_t f = td->type_flags;
        uint32_t vec = td->traits.numeric.vector.component_count;
        uint32_t cols = td->traits.numeric.matrix.column_count;
        uint32_t rows = td->traits.numeric.matrix.row_count;
        if (vec == 0) vec = 1;
        int isSigned = td->traits.numeric.scalar.signedness;

        if (f & SPV_REFLECT_TYPE_FLAG_MATRIX) {
            char b[32]; snprintf(b, sizeof(b), "float%ux%u", rows, cols); return b;
        }
        if (f & SPV_REFLECT_TYPE_FLAG_VECTOR) {
            if (f & SPV_REFLECT_TYPE_FLAG_FLOAT) {
                if (vec == 4) return "float4";
                if (vec == 3) return "float3";
                if (vec == 2) return "float2";
                return "float";
            }
            if (f & SPV_REFLECT_TYPE_FLAG_INT) {
                if (isSigned) {
                    if (vec == 4) return "int4";
                    if (vec == 3) return "int3";
                    if (vec == 2) return "int2";
                    return "int32_t";
                } else {
                    if (vec == 4) return "uint4";
                    if (vec == 3) return "uint3";
                    if (vec == 2) return "uint2";
                    return "uint32_t";
                }
            }
        }
        if (f & SPV_REFLECT_TYPE_FLAG_FLOAT) return "float";
        if (f & SPV_REFLECT_TYPE_FLAG_INT)    return isSigned ? "int32_t" : "uint32_t";
        if (f & SPV_REFLECT_TYPE_FLAG_STRUCT) return "/*struct*/";
        return "uint8_t";
    };

    // 递归计算成员大小
    std::function<size_t(const SpvReflectBlockVariable*)> memberDataSize =
        [&](const SpvReflectBlockVariable* mb) -> size_t {
        if (!mb) return 0;
        auto* td = mb->type_description;
        if (!td) return mb->size;
        uint32_t f = td->type_flags;
        if (f & SPV_REFLECT_TYPE_FLAG_STRUCT) {
            return mb->padded_size > 0 ? mb->padded_size : mb->size;
        }
        // array: stride * count
        if ((f & SPV_REFLECT_TYPE_FLAG_ARRAY) && td->traits.array.dims_count > 0) {
            size_t stride = mb->padded_size > 0 ? mb->padded_size : mb->size;
            // for runtime arrays, size=0 but stride gives element spacing
            if (stride == 0 && td->struct_type_description) {
                // array of struct — use parent block info
            }
            return stride;
        }
        // matrix
        if (f & SPV_REFLECT_TYPE_FLAG_MATRIX) {
            uint32_t c = td->traits.numeric.matrix.column_count;
            uint32_t r = td->traits.numeric.matrix.row_count;
            return c * r * 4;
        }
        // vector
        if (f & SPV_REFLECT_TYPE_FLAG_VECTOR) {
            return td->traits.numeric.vector.component_count * 4;
        }
        return mb->size > 0 ? mb->size : 4;
    };

    // 1) pipeline_ubo.h — UBO struct 定义 (std140 精确布局，支持嵌套 struct)
    {
        std::vector<SpvReflectShaderModule> aliveModules; // 生命周期延长到输出完成

        // 从 SPIR-V 解析指定 struct type 的 OpMemberDecorate → offset
        auto parseStructMemberOffsets = [](const std::vector<uint32_t>& spv, uint32_t structTypeId)
            -> std::map<uint32_t, uint32_t> // memberIndex → offset
        {
            std::map<uint32_t, uint32_t> offsets;
            for (size_t i = 5; i < spv.size(); ) {
                uint32_t wc = spv[i] >> 16;
                uint32_t op = spv[i] & 0xFFFF;
                if (wc == 0) { ++i; continue; }
                if (i + wc > spv.size()) break;
                // OpMemberDecorate = 72, DecorationOffset = 35
                if (op == 72 && wc >= 5 && spv[i+1] == structTypeId && spv[i+3] == 35) {
                    offsets[spv[i+2]] = spv[i+4]; // memberIdx → offset
                }
                i += wc;
            }
            return offsets;
        };

        // 从 SPIR-V 解析 ArrayStride decoration (OpDecorate %typeId ArrayStride value)
        auto parseArrayStride = [](const std::vector<uint32_t>& spv, uint32_t arrayTypeId) -> uint32_t {
            for (size_t i = 5; i < spv.size(); ) {
                uint32_t wc = spv[i] >> 16;
                uint32_t op = spv[i] & 0xFFFF;
                if (wc == 0) { ++i; continue; }
                if (i + wc > spv.size()) break;
                // OpDecorate=71, DecorationArrayStride=6
                if (op == 71 && wc >= 4 && spv[i+1] == arrayTypeId && spv[i+2] == 6)
                    return spv[i+3];
                i += wc;
            }
            return 0;
        };

        // 收集所有需要生成的 nested struct 类型 (去重), 按依赖顺序
        std::set<std::string> seenNames;
        std::vector<std::pair<std::string, const SpvReflectTypeDescription*>> nestedTypes; // name→typeDesc

        // 解引用 pointer/array → 拿到真正的 OpTypeStruct
        // block member 的 type_description 通常是 OpTypePointer, struct 在其 struct_type_description 里
        auto derefStructType = [](const SpvReflectTypeDescription* td)
            -> const SpvReflectTypeDescription* {
            if (!td) return nullptr;
            if ((td->type_flags & SPV_REFLECT_TYPE_FLAG_REF) && td->struct_type_description)
                td = td->struct_type_description;
            if ((td->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) && td->struct_type_description)
                td = td->struct_type_description;
            return (td->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) ? td : nullptr;
        };

        auto collectNestedTypes = [&](auto& self, const SpvReflectTypeDescription* td,
                                       const std::string& parentName, int depth) -> std::string {
            if (!td || depth > 8) return "uint8_t";
            uint32_t f = td->type_flags;

            // basic types (非 struct)
            if (!(f & SPV_REFLECT_TYPE_FLAG_STRUCT)) return spvTypeToStr(td);

            // array of struct → 递归到元素 struct, 不加 _arr 后缀
            if ((f & SPV_REFLECT_TYPE_FLAG_ARRAY) && td->struct_type_description) {
                return self(self, td->struct_type_description, parentName, depth);
            }

            // OpTypeStruct
            std::string nestedName = parentName + "_" + std::to_string(td->id);

            bool exists = false;
            for (auto& nt : nestedTypes)
                if (nt.first == nestedName) { exists = true; break; }
            if (!exists && td->member_count > 0) {
                nestedTypes.push_back({nestedName, td});
                // 递归收集更深层的嵌套 — 用 derefStructType 处理 pointer 成员
                for (uint32_t i = 0; i < td->member_count; ++i) {
                    auto* subStruct = derefStructType(&td->members[i]);
                    if (subStruct) {
                        self(self, subStruct, nestedName, depth + 1);
                    }
                }
            }
            return nestedName;
        };

        std::string uboPath = assetRoot + "pipeline_ubo.h";
        FILE* fu = fopen(uboPath.c_str(), "w");
        if (fu) {
            fprintf(fu, "// Auto-generated by ShaderCompiler — do not edit\n");
            fprintf(fu, "#pragma once\n#include <cstdint>\n");
            fprintf(fu, "// Slang types — 替换为你的数学库 (glm / hgl / custom)\n");
            fprintf(fu, "using float2 = struct { float x,y; };\n");
            fprintf(fu, "using float3 = struct { float x,y,z; };\n");
            fprintf(fu, "using float4 = struct { float x,y,z,w; };\n");
            fprintf(fu, "using float4x4 = float[16];\n");
            fprintf(fu, "using int2  = struct { int32_t x,y; };\n");
            fprintf(fu, "using int3  = struct { int32_t x,y,z; };\n");
            fprintf(fu, "using int4  = struct { int32_t x,y,z,w; };\n");
            fprintf(fu, "using uint2 = struct { uint32_t x,y; };\n");
            fprintf(fu, "using uint3 = struct { uint32_t x,y,z; };\n");
            fprintf(fu, "using uint4 = struct { uint32_t x,y,z,w; };\n\n");

            // --- 第一遍: 收集所有 UBO 和嵌套 struct 类型 ---
            struct UboInfo { std::string name; uint32_t set, bind; size_t totalSize;
                const SpvReflectBlockVariable* block; const std::vector<uint32_t>* spv; };
            std::vector<UboInfo> ubos;

            for (uint32_t s = 0; s <= (uint32_t)ugi::ShaderModuleType::ComputeShader; ++s) {
                if (SPIRV_DATA[s].empty()) continue;

                SpvReflectShaderModule m = {};
                if (spvReflectCreateShaderModule(SPIRV_DATA[s].size() * sizeof(uint32_t),
                        SPIRV_DATA[s].data(), &m) != SPV_REFLECT_RESULT_SUCCESS)
                    continue;
                aliveModules.push_back(m);

                uint32_t cnt = 0;
                spvReflectEnumerateDescriptorBindings(&m, &cnt, nullptr);
                std::vector<SpvReflectDescriptorBinding*> bindings(cnt);
                spvReflectEnumerateDescriptorBindings(&m, &cnt, bindings.data());

                for (auto* b : bindings) {
                    if (!b || (b->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                            && b->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC))
                        continue;
                    if (b->block.member_count == 0) continue;

                    std::string name = b->name ? b->name : "unnamed";
                    const char* pfx = "SLANG_ParameterGroup_";
                    if (name.compare(0, strlen(pfx), pfx) == 0) name = name.substr(strlen(pfx));
                    auto us = name.rfind("_std");
                    if (us != std::string::npos) name = name.substr(0, us);

                    std::string key = name + "@" + std::to_string(b->set) + ":" + std::to_string(b->binding);
                    if (!seenNames.insert(key).second) continue;

                    // 收集嵌套 struct 类型
                    for (uint32_t mi = 0; mi < b->block.member_count; ++mi) {
                        auto* structTd = derefStructType(b->block.members[mi].type_description);
                        if (structTd) {
                            collectNestedTypes(collectNestedTypes, structTd, name, 0);
                        }
                    }

                    size_t totalSize = b->block.padded_size;
                    if (totalSize == 0) totalSize = b->block.size;
                    // std140: uniform buffer struct 整体必须 16 字节对齐
                    totalSize = (totalSize + 15) & ~(size_t)15;
                    ubos.push_back({name, b->set, b->binding, totalSize, &b->block, &SPIRV_DATA[s]});
                }
            }

            // --- 输出嵌套 struct 定义 (反向: 深层依赖先输出) ---
            if (!nestedTypes.empty()) {
                std::set<std::string> emittedTypes;
                // 反向输出: 深层类型后加入 vector, 先输出它们
                for (auto it = nestedTypes.rbegin(); it != nestedTypes.rend(); ++it) {
                    const std::string& nestedName = it->first;
                    const SpvReflectTypeDescription* td = it->second;
                    if (!emittedTypes.insert(nestedName).second) continue;

                // 计算嵌套 struct 总大小
                size_t totalSize = 0;
                // 在所有 SPIR-V stage 中查找该 type 的 OpMemberDecorate
                std::map<uint32_t, uint32_t> memberOffsets;
                for (uint32_t ss = 0; ss <= (uint32_t)ugi::ShaderModuleType::ComputeShader; ++ss) {
                    if (SPIRV_DATA[ss].empty()) continue;
                    memberOffsets = parseStructMemberOffsets(SPIRV_DATA[ss], td->id);
                    if (!memberOffsets.empty()) break;
                }

                fprintf(fu, "// nested struct type\n");
                fprintf(fu, "struct %s {\n", nestedName.c_str());

                size_t expectedOff = 0;
                printf("  [nested] %s member_count=%u\n", nestedName.c_str(), td->member_count);
                for (uint32_t mi = 0; mi < td->member_count; ++mi) {
                    auto* mtd = &td->members[mi];
                    uint32_t off = 0;
                    auto it = memberOffsets.find(mi);
                    if (it != memberOffsets.end()) off = it->second;
                    printf("  [nested]   member[%u] off=%u flags=0x%x vec=%u arr=%u\n",
                        mi, off, mtd->type_flags,
                        mtd->traits.numeric.vector.component_count,
                        mtd->traits.array.dims_count > 0 ? mtd->traits.array.dims[0] : 0);

                    std::string mname = mtd->struct_member_name ? mtd->struct_member_name
                                            : "_m" + std::to_string(mi);
                    std::string tname;
                    auto* subStruct = derefStructType(mtd);
                    if (subStruct) {
                        tname = collectNestedTypes(collectNestedTypes, subStruct, nestedName, 0);
                    } else {
                        tname = spvTypeToStr(mtd);
                    }

                    bool isArray = (mtd->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY)
                                   && mtd->traits.array.dims_count > 0;
                    uint32_t arrCount = isArray ? mtd->traits.array.dims[0] : 1;

                    // element size
                    size_t elemSz = 4;
                    uint32_t vf = mtd->type_flags;
                    if (vf & SPV_REFLECT_TYPE_FLAG_MATRIX) {
                        uint32_t c = mtd->traits.numeric.matrix.column_count;
                        uint32_t r = mtd->traits.numeric.matrix.row_count;
                        elemSz = c * r * 4;
                    } else if (vf & SPV_REFLECT_TYPE_FLAG_VECTOR) {
                        elemSz = mtd->traits.numeric.vector.component_count * 4;
                    } else if (subStruct) {
                        elemSz = 16;
                    } else {
                        elemSz = 4;
                    }

                    if (off > expectedOff)
                        fprintf(fu, "    uint8_t _pad%u[%u];\n", mi, (unsigned)(off - expectedOff));

                    if (isArray && arrCount > 1) {
                        // 对 array-of-struct, 从 SPIR-V 读 ArrayStride 得到真实元素步长
                        if (subStruct && elemSz <= 16) {
                            for (uint32_t ss = 0; ss <= (uint32_t)ugi::ShaderModuleType::ComputeShader; ++ss) {
                                if (SPIRV_DATA[ss].empty()) continue;
                                uint32_t stride = parseArrayStride(SPIRV_DATA[ss], mtd->id);
                                if (stride > 0) { elemSz = stride; break; }
                            }
                        }
                        fprintf(fu, "    %-16s %s[%u];\n", tname.c_str(), mname.c_str(), arrCount);
                        expectedOff = off + arrCount * (elemSz > 0 ? elemSz : 16);
                    } else {
                        fprintf(fu, "    %-16s %s;\n", tname.c_str(), mname.c_str());
                        expectedOff = off + elemSz;
                    }
                }
                // 推断 totalSize 并对齐到 16 字节 (std140)
                if (totalSize == 0) totalSize = expectedOff;
                size_t rawTotal = totalSize;
                totalSize = (totalSize + 15) & ~(size_t)15;
                printf("  [nested] %s expectedOff=%zu rawTotal=%zu alignedTotal=%zu\n",
                    nestedName.c_str(), expectedOff, rawTotal, totalSize);
                if (totalSize > expectedOff)
                    fprintf(fu, "    uint8_t _endPad[%zu];\n", totalSize - expectedOff);
                fprintf(fu, "};\n\n");
            }
            }

            // --- 输出顶层 UBO struct ---
            for (auto& ubo : ubos) {
                fprintf(fu, "// \"%s\"  set=%u bind=%u  size=%zu\n",
                    ubo.name.c_str(), (unsigned)ubo.set, (unsigned)ubo.bind, ubo.totalSize);
                fprintf(fu, "struct %s_UBO {\n", ubo.name.c_str());

                size_t expectedOff = 0;
                for (uint32_t mi = 0; mi < ubo.block->member_count; ++mi) {
                    auto* mb = &ubo.block->members[mi];
                    uint32_t off = mb->offset;
                    printf("  [ubo]   member[%u] name='%s' offset=%u size=%u padded=%u\n",
                        mi, mb->name ? mb->name : "(null)", off, mb->size, mb->padded_size);
                    std::string mname = mb->name ? mb->name : "_m" + std::to_string(mi);
                    auto* td = mb->type_description;

                    // type name: use nested type if struct
                    std::string tname;
                    auto* structTd = derefStructType(td);
                    if (structTd) {
                        tname = collectNestedTypes(collectNestedTypes, structTd, ubo.name, 0);
                    } else {
                        tname = spvTypeToStr(td);
                    }

                    if (off > expectedOff)
                        fprintf(fu, "    uint8_t _pad%u[%u];\n", mi, (unsigned)(off - expectedOff));

                    bool isArray = td && (td->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY)
                                   && td->traits.array.dims_count > 0;
                    uint32_t arrCount = isArray ? td->traits.array.dims[0] : 1;

                    // mb->size: 对基础类型=元素大小, 对数组=总大小, 可能=0
                    // padded_size 对尾部成员包含 struct 尾部对齐填充, 不能当元素大小用
                    size_t elemSz = mb->size;
                    if (elemSz == 0 && td) {
                        if (td->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
                            elemSz = td->traits.numeric.matrix.column_count * td->traits.numeric.matrix.row_count * 4;
                        else if (td->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR)
                            elemSz = td->traits.numeric.vector.component_count * 4;
                        else
                            elemSz = 4;
                    }
                    if (elemSz == 0) elemSz = 16;
                    // 仅嵌套 struct 成员用 padded_size (std140 对齐后的大小)
                    if (structTd && mb->padded_size > elemSz)
                        elemSz = mb->padded_size;

                    if (isArray && arrCount > 1) {
                        fprintf(fu, "    %-16s %s[%u];\n", tname.c_str(), mname.c_str(), arrCount);
                        expectedOff = off + arrCount * elemSz;
                    } else if (tname == "uint8_t") {
                        size_t remain = ubo.totalSize > off ? ubo.totalSize - off : elemSz;
                        if (remain > 0) fprintf(fu, "    uint8_t     %s[%zu];\n", mname.c_str(), remain);
                        expectedOff = off + remain;
                    } else {
                        fprintf(fu, "    %-16s %s;\n", tname.c_str(), mname.c_str());
                        expectedOff = off + elemSz;
                    }
                }
                if (ubo.totalSize > expectedOff) {
                    fprintf(fu, "    uint8_t _endPad[%zu];\n", ubo.totalSize - expectedOff);
                }
                printf("  [ubo] %s totalSize=%zu expectedOff=%zu\n", ubo.name.c_str(), ubo.totalSize, expectedOff);

                fprintf(fu, "};\n");
                fprintf(fu, "static_assert(sizeof(%s_UBO) == %zu, \"size mismatch\");\n\n",
                    ubo.name.c_str(), ubo.totalSize);
            }
            fclose(fu);
            printf("[gen] %s\n", uboPath.c_str());
        }

        // 所有输出完成后再释放 SPIRV-Reflect 模块
        for (auto& m : aliveModules) {
            spvReflectDestroyShaderModule(&m);
        }
    }

    // 2) pipeline_bindings.h — 绑定名常量
    {
        std::string bindPath = assetRoot + "pipeline_bindings.h";
        FILE* fb = fopen(bindPath.c_str(), "w");
        if (fb) {
            fprintf(fb, "// Auto-generated by ShaderCompiler — do not edit\n");
            fprintf(fb, "#pragma once\n\n");
            for (uint32_t i = 0; i < ugi::MaxArgumentCount; ++i) {
                auto& layout = desc.argumentLayouts[i];
                if (layout.descriptorCount) {
                    for (uint32_t j = 0; j < ugi::MaxDescriptorCount; ++j) {
                        auto& d = layout.descriptors[j];
                        if (d.binding != 0xff) {
                            char upper[64];
                            auto toUpper = [](char* s, const char* src) {
                                int k = 0;
                                for (; src[k] && k < 62; ++k)
                                    s[k] = (src[k] >= 'a' && src[k] <= 'z') ? src[k] - 32 : src[k];
                                s[k] = 0;
                            };
                            toUpper(upper, d.name);
                            fprintf(fb, "#define BIND_%s \"%s\"\n", upper, d.name);
                        }
                    }
                }
            }
            fclose(fb);
            printf("[gen] %s\n", bindPath.c_str());
        }
    }

    return 0;
}
