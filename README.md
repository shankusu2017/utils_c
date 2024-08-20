# utils_c

utils by c language in linux

### TODO

- #### threadpool 代码优化

  - 确定性能瓶颈在哪里
- #### Hash 表 size 动态调整

  - 不影响现有接口的情况下，实现 size 动态调整(参考下 redis ？)
- ### IPC

  - 尝试记录 sync_send 中，内存未释放的行为
  - client 与 server 断开后自动重连
  - client 链接状态的使用
  - server 中数据 mutex 保护(cli_hash等)
