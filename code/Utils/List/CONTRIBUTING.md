# Contributing to Embedded-List

感谢您对 Embedded-List 项目的关注！我们欢迎所有形式的贡献。

## 如何贡献

### 报告问题

如果您发现了 bug 或有功能建议，请通过以下方式提交：

1. 在 GitHub Issues 中创建新 issue
2. 提供清晰的问题描述和复现步骤
3. 如果是 bug，请提供：
   - 使用的编译器版本
   - 目标平台（FreeRTOS/CMSIS-RTOS/Windows/其他）
   - 最小可复现的代码示例

### 提交代码

1. **Fork 仓库**
   ```bash
   git clone https://github.com/your-username/embedded-list.git
   cd embedded-list
   ```

2. **创建功能分支**
   ```bash
   git checkout -b feature/your-feature-name
   # 或
   git checkout -b fix/your-bug-fix
   ```

3. **编写代码**
   - 遵循现有的代码风格
   - 添加必要的注释
   - 确保代码通过编译（无警告）
   - 如果是新功能，请添加相应的单元测试

4. **运行测试**
   ```bash
   gcc -o test_main test_main.c test_list.c embedded_list.c list_save.c -I.
   ./test_main
   ```

5. **提交更改**
   ```bash
   git add .
   git commit -m "描述你的更改"
   git push origin feature/your-feature-name
   ```

6. **创建 Pull Request**
   - 在 GitHub 上创建 Pull Request
   - 描述你的更改和原因
   - 确保所有测试通过

## 代码规范

### C 代码风格

- 使用 4 个空格缩进（不使用 Tab）
- 函数名使用 `snake_case`
- 类型名使用 `snake_case` 并以 `_t` 结尾
- 宏定义使用 `UPPER_CASE`
- 添加必要的注释，特别是公共 API

### 示例

```c
// 好的风格
list_t *list_create(uint16_t capacity, uint16_t element_size)
{
    if (capacity == 0 || element_size == 0)
        return NULL;
    // ...
}

// 避免的风格
list_t* list_create(uint16_t capacity,uint16_t element_size){
    if(capacity==0||element_size==0)return NULL;
    // ...
}
```

## 测试要求

- 新功能必须包含单元测试
- 所有测试必须通过
- 测试代码应该清晰易懂
- 测试应该覆盖边界情况

## 文档要求

- 更新 README.md（如果添加了新功能）
- 更新 API 文档
- 添加使用示例（如果适用）
- 更新 CHANGELOG.md

## 许可证

通过贡献代码，您同意您的贡献将在 MIT 许可证下发布。

## 问题？

如果您有任何问题，请通过 GitHub Issues 联系我们。

感谢您的贡献！🎉

