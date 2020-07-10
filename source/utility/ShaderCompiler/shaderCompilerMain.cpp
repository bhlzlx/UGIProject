#include <cstdio>
#include "json.hpp"
#include "ShaderCompilerUtility.h"
#include <regex>
#include <string>
#include <cassert>
#include <vector>
#include <cstdint>
#include <hgl/assets/AssetsSource.h>
#include <hgl/io/OutputStream.h>
#include <hgl/io/FileOutputStream.h>
#include <hgl/filesystem/FileSystem.h>
#include <hgl/CodePage.h>
#include <unordered_map>

std::array<std::vector<uint32_t>, (uint32_t)ugi::ShaderModuleType::ComputeShader+1 >  SPIRV_DATA;

bool ProcessSpirVForStage( const char* shaderText, EShLanguage stage, ugi::PipelineDescription& desc ) {
    
    EShLanguage shaderStage = stage;
    glslang::TShader Shader(shaderStage);
    Shader.setStrings(&shaderText, 1);
    int ClientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100
    glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_0;  // would map to, say, Vulkan 1.0
    glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;    // maps to, say, SPIR-V 1.0

    Shader.setEnvInput(glslang::EShSourceGlsl, shaderStage, glslang::EShClientVulkan, ClientInputSemanticsVersion);
    Shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
    Shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);

    TBuiltInResource Resources;
    Resources = DefaultTBuiltInResource;
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    const int DefaultVersion = 100;

    glslang::TShader::ForbidIncluder includer;

    std::string PreprocessedGLSL;
    bool res = Shader.preprocess(&Resources, DefaultVersion, ENoProfile, false, false, messages, &PreprocessedGLSL, includer);
    if (!res) {
        return false;
    }
    const char* PreprocessedCStr = PreprocessedGLSL.c_str();
    Shader.setStrings(&PreprocessedCStr, 1);

    std::string compileMessage;

    res = Shader.parse(&Resources, 100, false, messages);
    if (!res) {
        const char* pLog = Shader.getInfoLog();
        const char* pDbgLog = Shader.getInfoDebugLog();
        compileMessage.clear();
        compileMessage += "Info :";
        compileMessage += pLog;
        compileMessage += "\r\n";
        compileMessage += "Debug :";
        compileMessage += pDbgLog;
        compileMessage += "\r\n";
        return false;
    }
    glslang::TProgram Program;
    Program.addShader(&Shader);
    res = Program.link(messages);
    assert(res == true);
    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    ugi::ShaderModuleType shaderModule = shaderStageConvert( stage );
    desc.shaders[(uint32_t)shaderModule].type = shaderModule;
    std::vector<uint32_t>& compiledSPV = SPIRV_DATA[(uint32_t)shaderModule];
    //
    glslang::GlslangToSpv(*Program.getIntermediate(shaderStage), compiledSPV, &logger, &spvOptions);

    

    spirv_cross::Parser spvParser(compiledSPV.data(), compiledSPV.size());
    spvParser.parse();
    spirv_cross::Compiler spvCompiler(spvParser.get_parsed_ir());
    
    spirv_cross::ShaderResources spvResource = spvCompiler.get_shader_resources();

    if( stage == EShLangVertex) {
        for (auto& stageInput : spvResource.stage_inputs) {
            auto location = spvCompiler.get_decoration(stageInput.id, spv::Decoration::DecorationLocation);
            auto type = spvCompiler.get_type(stageInput.base_type_id);
            strcpy(desc.vertexLayout.attributes[location].name, stageInput.name.c_str());
            desc.vertexLayout.attributes[location].type = getVertexType(type);
            ++desc.vertexLayout.attributeCount;
        }
    }
    // m_vecStageOutput.clear();
    // for (auto& stageOutput : spvResource.stage_outputs) {
    //     auto location = get_decoration(stageOutput.id, spv::Decoration::DecorationLocation);
    //     StageIOAttribute attr;
    //     attr.location = location;
    //     strcpy(attr.name, stageOutput.name.c_str());
    //     auto& type = get<spirv_cross::SPIRType>(stageOutput.base_type_id);
    //     attr.type = getVertexType(type);
    //     m_vecStageOutput.push_back(attr);
    // }
    //
    //m_pushConstants.offset = 0;
    //m_pushConstants.size = 0;
    //
    if (spvResource.push_constant_buffers.size()) {
        auto& constants = spvResource.push_constant_buffers[0];
        auto ranges = spvCompiler.get_active_buffer_ranges(constants.id);
        desc.pipelineConstants[(uint32_t)shaderModule].offset = ranges[0].offset;
        desc.pipelineConstants[(uint32_t)shaderModule].size = ranges.back().offset - ranges[0].offset + ranges.back().range;
    }
    // ======================= buffer type ===========================
    for (auto& uniformBlock : spvResource.uniform_buffers) {
        UniformBuffer block;
        block.set = spvCompiler.get_decoration(uniformBlock.id, spv::Decoration::DecorationDescriptorSet);
        block.binding = spvCompiler.get_decoration(uniformBlock.id, spv::Decoration::DecorationBinding);
        strcpy(block.name, uniformBlock.name.c_str());
        //const auto &type = get_type(uniformBlock.base_type_id);
        const auto& spirType = spvCompiler.get_type_from_variable(uniformBlock.id);
        size_t blockSize = spvCompiler.get_declared_struct_size(spirType);
        block.size = blockSize;
        desc.argumentLayouts[block.set].descriptors[block.binding].binding = block.binding;
        desc.argumentLayouts[block.set].descriptors[block.binding].dataSize = block.size;
        desc.argumentLayouts[block.set].descriptors[block.binding].shaderStage = shaderModule;
        desc.argumentLayouts[block.set].descriptors[block.binding].type = ugi::ArgumentDescriptorType::UniformBuffer;
        strcpy( desc.argumentLayouts[block.set].descriptors[block.binding].name, block.name);
        ++desc.argumentLayouts[block.set].descriptorCount;
    }

    for (auto& item : spvResource.storage_buffers) {
        StorageBuffer block;
        block.binding = spvCompiler.get_decoration(item.id, spv::Decoration::DecorationBinding);
        block.set = spvCompiler.get_decoration(item.id, spv::Decoration::DecorationDescriptorSet);
        strcpy(block.name, item.name.c_str());
        auto spirType = spvCompiler.get_type_from_variable(item.id);
        block.size = spvCompiler.get_declared_struct_size(spirType);
        //
        desc.argumentLayouts[block.set].descriptors[block.binding].binding = block.binding;
        desc.argumentLayouts[block.set].descriptors[block.binding].dataSize = block.size;
        desc.argumentLayouts[block.set].descriptors[block.binding].shaderStage = shaderModule;
        desc.argumentLayouts[block.set].descriptors[block.binding].type = ugi::ArgumentDescriptorType::StorageBuffer;
        strcpy( desc.argumentLayouts[block.set].descriptors[block.binding].name, block.name);
        ++desc.argumentLayouts[block.set].descriptorCount;
    }
    // ======================= image type ===========================
    // 4 type : sampler/sampled image/storage image/input attachment
    for (auto& sampler : spvResource.separate_samplers) {
        SeparateSampler sam;
        sam.binding = spvCompiler.get_decoration(sampler.id, spv::Decoration::DecorationBinding);
        sam.set = spvCompiler.get_decoration(sampler.id, spv::Decoration::DecorationDescriptorSet);
        strcpy(sam.name, sampler.name.c_str());

        ++desc.argumentLayouts[sam.set].descriptorCount;
        desc.argumentLayouts[sam.set].descriptors[sam.binding].binding = sam.binding;
        desc.argumentLayouts[sam.set].descriptors[sam.binding].shaderStage = shaderModule;
        desc.argumentLayouts[sam.set].descriptors[sam.binding].type = ugi::ArgumentDescriptorType::Sampler;
        strcpy( desc.argumentLayouts[sam.set].descriptors[sam.binding].name, sam.name);
    }
    for (auto& image : spvResource.separate_images) {
        SeparateImage img;
        img.binding = spvCompiler.get_decoration(image.id, spv::Decoration::DecorationBinding);
        img.set = spvCompiler.get_decoration(image.id, spv::Decoration::DecorationDescriptorSet);
        strcpy(img.name, image.name.c_str());
        //
        ++desc.argumentLayouts[img.set].descriptorCount;
        desc.argumentLayouts[img.set].descriptors[img.binding].binding = img.binding;
        desc.argumentLayouts[img.set].descriptors[img.binding].shaderStage = shaderModule;
        desc.argumentLayouts[img.set].descriptors[img.binding].type = ugi::ArgumentDescriptorType::Image;
        strcpy( desc.argumentLayouts[img.set].descriptors[img.binding].name, img.name);
    }
    for (auto& sampledImage : spvResource.sampled_images) {
        assert( false && "unsupported descriptor!");
        // CombinedImageSampler image;
        // image.binding = get_decoration(sampledImage.id, spv::Decoration::DecorationBinding);
        // image.set = get_decoration(sampledImage.id, spv::Decoration::DecorationDescriptorSet);
        // strcpy(image.name, sampledImage.name.c_str());
        // m_vecCombinedImageSampler.push_back(image);
        // addDescriptorRecord(image.set, image.binding);
    }
    for (auto& storageImage : spvResource.storage_images) {
        StorageImage image;
        image.binding = spvCompiler.get_decoration(storageImage.id, spv::Decoration::DecorationBinding);
        image.set = spvCompiler.get_decoration(storageImage.id, spv::Decoration::DecorationDescriptorSet);
        strcpy(image.name, storageImage.name.c_str());
        //
        ++desc.argumentLayouts[image.set].descriptorCount;
        desc.argumentLayouts[image.set].descriptors[image.binding].binding = image.binding;
        desc.argumentLayouts[image.set].descriptors[image.binding].shaderStage = shaderModule;
        desc.argumentLayouts[image.set].descriptors[image.binding].type = ugi::ArgumentDescriptorType::StorageImage;
        strcpy( desc.argumentLayouts[image.set].descriptors[image.binding].name, image.name);
    }
    //
    for (auto& pass : spvResource.subpass_inputs) {
        InputAttachment input;
        input.set = spvCompiler.get_decoration(pass.id, spv::Decoration::DecorationDescriptorSet);
        input.binding = spvCompiler.get_decoration(pass.id, spv::Decoration::DecorationBinding);
        input.inputIndex = spvCompiler.get_decoration(pass.id, spv::Decoration::DecorationIndex);
        strcpy(input.name, pass.name.c_str());
        //
        ++desc.argumentLayouts[input.set].descriptorCount;
        desc.argumentLayouts[input.set].descriptors[input.binding].binding = input.binding;
        desc.argumentLayouts[input.set].descriptors[input.binding].shaderStage = shaderModule;
        desc.argumentLayouts[input.set].descriptors[input.binding].type = ugi::ArgumentDescriptorType::InputAttachment;
        strcpy( desc.argumentLayouts[input.set].descriptors[input.binding].name, input.name);
    }
    return true;
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

int main( int argc, char** argv ) {

    ugi::PipelineDescription pipelineDescription;

    const char expr[] = R"(^(.*)\\[_0-9a-zA-Z]*.exe$)";
    std::regex pathRegex(expr);
    std::smatch result;
    std::string content(argv[0]);
    bool succees = std::regex_match(content, result, pathRegex);
    std::string assetRoot;
    if (succees) {
        assetRoot = result[1];
    }
    if(!argv[1]) {
        return -1;
    }
    assetRoot.append( std::string("/../shaders/") + argv[1] );

    hgl::assets::AssetsSource* assetsSource = hgl::assets::CreateSourceByFilesystem( "source", hgl::ToOSString(assetRoot.c_str()), true );

    // kwheel::IArchive* archive = kwheel::CreateStdArchieve(assetRoot);

    if( !glslang::InitializeProcess() ) {
        printf("initialize glslang failed!\n");
    }

    std::string configPath = "pipeline.json";

    auto jsonAsset = assetsSource->Open(configPath.c_str());
    char* jsonBuffer = new char[jsonAsset->GetSize()+1];
    jsonAsset->Read(jsonBuffer, jsonAsset->GetSize());
    jsonBuffer[jsonAsset->GetSize()] = 0;
    nlohmann::json js = nlohmann::json::parse(jsonBuffer);
    delete[] jsonBuffer;

    std::string shaderFile;

    auto shaderModule = js["shaderModule"];
    if( shaderModule.is_null() ) {
        printf("shader module was empty!\n");
        return -1;
    } else {

        const std::map< const char*, EShLanguage > shaderStageTable = {
            {"vert", EShLanguage::EShLangVertex},
            {"tesc", EShLanguage::EShLangTessControl},
            {"tese", EShLanguage::EShLangTessEvaluation},
            {"geom", EShLanguage::EShLangGeometry},
            {"frag", EShLanguage::EShLangFragment},
            {"comp", EShLanguage::EShLangCompute},
        };

        for( auto& module : shaderStageTable ) {
            if( shaderModule[module.first].is_string()) {
                shaderFile = shaderModule[module.first];
                auto shaderAsset = assetsSource->Open(shaderFile.c_str());
                char* shaderText = new char[shaderAsset->GetSize()+1];
                shaderAsset->Read(shaderText, shaderAsset->GetSize());
                shaderText[shaderAsset->GetSize()] = 0;
                ProcessSpirVForStage( shaderText, module.second, pipelineDescription );
                delete[] shaderText;
            }
        }
    }

    uint32_t offsetCounter = sizeof(ugi::PipelineDescription);
    for( uint32_t i = 0; i<=(uint32_t)ugi::ShaderModuleType::ComputeShader; ++i) {
        if( SPIRV_DATA[i].size()) {
            pipelineDescription.shaders[i].spirvData = offsetCounter;
            pipelineDescription.shaders[i].spirvSize = SPIRV_DATA[i].size() * sizeof(uint32_t);
            offsetCounter += SPIRV_DATA[i].size();
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
        if(!a.descriptorCount) {
            avalue |= 0x10000000;
        }
        uint32_t bvalue = (0x1 << b.index);
        if(!b.descriptorCount) {
            avalue |= 0x10000000;
        }
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
    // 处理vertex layout
    ///> vertex layout information

    auto vertexLayout = js["vertexLayout"];
    if(vertexLayout.is_null()) {
        /// 没有顶点信息，生成默认布局
        uint32_t attrOffset = 0;
        for( uint32_t i = 0; i<pipelineDescription.vertexLayout.attributeCount; ++i) {
            pipelineDescription.vertexLayout.attributes[i].offset = attrOffset;
            attrOffset+= getVertexSize(pipelineDescription.vertexLayout.attributes[i].type);
            pipelineDescription.vertexLayout.attributes[i].bufferIndex = 0;
        }
        // 生成默认配置，只有一个buffer的情况
        pipelineDescription.vertexLayout.bufferCount = 1;
        pipelineDescription.vertexLayout.buffers[0].stride = attrOffset;
        pipelineDescription.vertexLayout.buffers[0].instanceMode = 0;
    } else {
        if(!vertexLayout["buffers"].is_array()) {
            printf("buffers segment fault!\n");
            return -1;
        } else {
            auto buffers = vertexLayout["buffers"];
            pipelineDescription.vertexLayout.bufferCount = 0;
            for( auto buffer : buffers ) {
                if( !buffer["stride"].is_number_integer() || !buffer["instance"].is_number_integer() ) {
                    printf("buffer attribute is fault!\n");
                    return -1;
                }
                pipelineDescription.vertexLayout.buffers[pipelineDescription.vertexLayout.bufferCount].instanceMode = buffer["instance"];
                pipelineDescription.vertexLayout.buffers[pipelineDescription.vertexLayout.bufferCount].stride = buffer["stride"];
                ++pipelineDescription.vertexLayout.bufferCount;
            }
        }
        if(!vertexLayout["attributes"].is_array()) {
            printf("vertex attributes fault!\n");
            return -1;
        } else {
            auto attributes = vertexLayout["attributes"];
            size_t i = 0;
            pipelineDescription.vertexLayout.attributeCount = 0;
            for( auto attr : attributes ) {
                if( !attr["buffer"].is_number_integer() || !attr["offset"].is_number_integer() || !attr["type"].is_string() ) {
                    printf("buffer attribute is fault!\n");
                    return -1;
                }
                pipelineDescription.vertexLayout.attributes[pipelineDescription.vertexLayout.attributeCount].bufferIndex = attr["buffer"];
                pipelineDescription.vertexLayout.attributes[pipelineDescription.vertexLayout.attributeCount].offset = attr["offset"];
                pipelineDescription.vertexLayout.attributes[pipelineDescription.vertexLayout.attributeCount].type = VertexTypeMapping[attr["type"]];
                ++pipelineDescription.vertexLayout.attributeCount;
            }
        }
    }

    hgl::io::OpenFileOutputStream fileOutput( hgl::ToOSString( (assetRoot + "/pipeline.bin").c_str()) );
    hgl::io::FileOutputStream* stream = fileOutput;
    stream->Write(&pipelineDescription, sizeof(pipelineDescription));
    for(auto spv : SPIRV_DATA){
        if(spv.size()) {
            stream->Write(spv.data(), spv.size()*sizeof(uint32_t));
        }
    }
    
    // char filename [] = "0.bin";
    // for(auto spv : SPIRV_DATA){
    //     if(spv.size()) {
    //         auto x = archive->open(filename,
    //             (uint32_t)kwheel::OpenFlagBits::Binary | 
    //             (uint32_t)kwheel::OpenFlagBits::Write
    //         );
    //         x->write(spv.size()*sizeof(uint32_t), spv.data());
    //         ++filename[0];
    //         x->release();
    //     }
    // }
    return 0;
}