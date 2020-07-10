# Shader Compiler

## 实现目标

1. 读取配置文件，编译生成 若干个 SPIR-V 二进制文件
2. 将所有shader的信息拿出来，整合成 *Pipeline Description*，便于拿出来给程序直接用