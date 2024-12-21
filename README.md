# utils_c

utils by c language in unix like

## 设计目标

* c 语言的脚手架

## TODO
- ### threadpool 代码优化

- ### Hash 表 size 动态调整

  - 不影响现有接口的情况下，实现 size 动态调整(参考下 redis ？)
- ### IPC

  - 尝试记录 sync_send 中，内存未释放的行为
  - 提高性能(io)

- ### Timer test
  - 完善测试用例

- ### 版本整理
  - 整理头文件，发布v0.1版本