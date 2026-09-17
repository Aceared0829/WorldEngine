# Make sure this project is built when the Editor is built
W_add_as_runtime_dependency(ShaderCompilerWebGPU)

W_add_dependency("ShaderCompiler" "ShaderCompilerWebGPU")