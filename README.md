https://blog.csdn.net/qq_31562655/article/details/121615213

User/
├── App/                  -- 应用层代码             
│   ├── SystemTasks/      -- 系统任务
│   │   ├── DebugTask     -- 调试任务
│   │   ├── WatchdogTask  -- 看门狗任务
│   │   ├── CanTask       -- CAN协议解析
│   │   ├── UartTask      -- 串口协议解析
│   |   └── DataLogger    -- 数据记录服务
│   └── UserTasks/        -- 用户业务任务
│       ├── ControlTask
│       └── MonitorTask
├── BSP/                      -- 板级支持包
│   ├── Devices/              -- 设备驱动抽象
│   │   ├── Motor/            -- 电机驱动
│   │   │   ├── Maxon/        -- 品牌1实现
│   │   │   └── Teknic/       -- 品牌2实现
│   │   ├── H_Bridge/         -- H桥控制
│   │   ├── Remote/           -- 遥控器驱动
│   │   └── IMU/              -- 陀螺仪驱动
│   └── BSP_Common.h          -- 设备公共接口
├── Middlewares/              -- 中间件层
│   ├── Algorithm/            -- 算法实现
│   │   ├── PID/              -- PID控制器
│   │   ├── Kalman/           -- 卡尔曼滤波
│   │   └── MathLib/          -- 数学计算库
│   └── Utilities/            -- 工具类
│       ├── MemAlloc/         -- 自定义内存分配
│       │   ├── mem_pool.c
│       │   └── mem_pool.h
│       └── Debug/            -- 调试工具
│           ├── printf_redirect.c
│           └── debug_utils.h
├── System/                   -- 系统级组件
└── Config/                  -- 工程配置
    ├── sys_config.h         -- 系统配置头文件
    └── task_config.h        -- 任务参数配置