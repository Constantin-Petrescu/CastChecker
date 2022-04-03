# Cast extractor - Clang plugin


We wrote a clang plugin to extract information about the uses of named cast operators in C++ code. 

The clang was built based on: 
- Documentation provided by clang in: https://clang.llvm.org/docs/ClangPlugins.html
- PrintFunctionName plugin from: https://github.com/llvm/llvm-project/blob/main/clang/examples/PrintFunctionNames/PrintFunctionNames.cpp

### Dependencies
Clang-10
llvm-10.0.0svn

### Build and Usage
1 Follow the tutorial from https://clang.llvm.org/get_started.html to build a local clang/llvm. Ensure to build examples for clangs (we used `-DLLVM_BUILD_EXAMPLES=1 -DCLANG_BUILD_EXAMPLES=1` - the build system and their commands probably changed for the newer versions).
2 Copy and paste the clang plugin into the following path of clang plugin: `/path/to/clang/clang/examples` . 
3 After, pasting the plugin we need to specify into the CMakeLists.txt the directory of the plugin in order to give access for clang to build the plugin `add_subdirectory(plugin_dir)`
4 To run the clang plugin, execute clang and append the following arguments while building a C++ file: `-Xclang -load -Xclang extract_casts.dylib -Xclang -plugin -Xclang extract-casts`

### Disclaimer
We can not guarantee the build/execution of the clang plugin on newer versions of clang/llvm. 



