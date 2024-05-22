# utils_c
utils by c language in linux



### TODO

- #### threadpool 代码优化

  - 确定性能瓶颈在哪里
  - 能否减少 mutex 的数量和复杂度
  - 能否将管理线程取消，融入到任务线程中（线程调度的是否自动处理）
  - 任务列表是否能改为队列模式（FIFO）
