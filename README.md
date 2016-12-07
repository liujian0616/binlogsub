# mysqlrep
解析mysql的binlog，并且把解出来的数据写入redis。维护一块和mysql库中数据同步的缓存

改自mybus： https://github.com/liudong1983/mybus

做了以下改动
1.mybus太大了，而且不容易跑起来，这里主要把他简化了，只留了解析binlog的逻辑
2.增加了对mysql5.6的支持

目前代码运行在mysql5.6环境比较稳定，在Mariadb环境运行时会出现一些问题
