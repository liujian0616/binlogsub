#binlogsub

v0.1

解析mysql的binlog，并且把解出来的数据写入redis。维护一块和mysql库中数据同步的缓存

改自mybus： https://github.com/liudong1983/mybus

做了以下改动

1.mybus太大了，而且不容易跑起来，这里主要把他简化了，只留了解析binlog的逻辑

2.增加了对mysql5.6的支持

目前代码运行在mysql5.6环境比较稳定，在Mariadb环境运行时会出现一些问题


v0.2

v0.1 中的代码是在mybus基础上改的，本版中代码是全部重写了。而且将业务代码也去掉了。只保留了解析binlog的代码，并打印出配置文件中配好的列数据

注意：配置文件 column_index 很重要，表示数据库中这一列的 的序号，从0开始计算
