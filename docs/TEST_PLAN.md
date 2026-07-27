# Openterface_Core 测试完善方案

## 一、现状分析

### 当前测试结果
| 测试文件 | 状态 | 通过率 |
|---------|------|--------|
| `input_test.c` | ✅ 通过 | 100% |
| `native_core_test.c` | ❌ 失败 | 部分通过 |
| `watchdog_test.c` | ❌ 失败 | 部分通过 |
| `core_skeleton_test.c` | ❌ 失败 | 部分通过 |

### 失败原因
1. **native_core_test**: 宽度归一化期望 3840 实际 7680，串口端点/后端设置目标模式失败
2. **watchdog_test**: 状态机转换问题，错误累积后未正确进入 DISCONNECTED 状态
3. **core_skeleton_test**: USB 开关响应解析校验和计算范围错误（测试算 7 字节，代码期望 6 字节）

### 未覆盖模块
- `profile.c` - 设备配置文件管理
- `device_info.c` - 设备信息初始化
- `hid_device_session.c` - HID 设备会话管理
- `transport.c` - 传输层实现
- `chip_detector.c` - 芯片检测
- `usb_mode_controller.c` - USB 模式控制

---

## 二、修复计划（优先级排序）

### Phase 1: 修复现有失败测试（P0）

#### 1.1 core_skeleton_test.c - USB 开关测试
**问题**: 校验和计算范围错误
**修复**: 将校验和计算从 7 字节改为 6 字节
```c
// 错误：response[6] = op_ch9329_checksum(response, 7);
// 正确：response[6] = op_ch9329_checksum(response, 6);
```

#### 1.2 native_core_test.c - 宽度归一化
**问题**: 期望值与实际值不匹配
**修复**: 检查 `video_chip_controller.c` 中的归一化逻辑，更新测试期望值或修复实现

#### 1.3 watchdog_test.c - 状态机转换
**问题**: 错误累积后状态未正确转换
**修复**: 检查 `connection_watchdog.c` 中恢复逻辑，确保达到最大恢复次数后正确设置 DISCONNECTED 状态

### Phase 2: 新增核心模块测试（P1）

#### 2.1 profile_test.c
测试设备配置文件管理：
- `op_profile_find_by_id()` - 按 ID 查找
- `op_profile_match()` - 按 VID/PID/接口匹配
- `op_profile_has_capability()` - 能力检查
- `op_profile_apply_to_device()` - 应用配置到设备

#### 2.2 device_info_test.c
测试设备信息管理：
- `op_device_info_init()` - 初始化
- `op_device_info_has_interface()` - 接口检查
- `op_device_info_has_capability()` - 能力检查

#### 2.3 transport_test.c
测试传输层：
- `op_transport_init()` - 初始化
- `op_transport_open/close()` - 开关
- `op_transport_read/write()` - 读写
- 使用 stub 后端测试

#### 2.4 hid_device_session_test.c
测试 HID 设备会话：
- `op_hid_device_session_create()` - 创建
- `op_hid_device_session_open/close()` - 开关
- `op_hid_device_session_send_feature_report()` - 发送报告
- 使用 mock transport 测试

### Phase 3: 新增高级功能测试（P2）

#### 3.1 chip_detector_test.c
- 芯片检测逻辑
- 不同芯片类型识别

#### 3.2 usb_mode_controller_test.c
- USB 模式切换
- 模式状态查询

#### 3.3 video_status_poller_test.c
- 视频状态轮询
- 状态缓存更新

### Phase 4: 测试基础设施改进（P3）

#### 4.1 统一测试框架
- 提取公共断言宏到 `tests/test_assert.h`
- 创建测试辅助函数库 `tests/test_helpers.c`
- 添加测试 setup/teardown 框架

#### 4.2 Mock 框架
- 创建 transport mock 实现
- 创建 HID 设备 mock
- 支持注入依赖测试

---

## 三、实施步骤

### Step 1: 修复失败测试
```bash
# 1. 修复 core_skeleton_test.c 校验和
# 2. 修复 native_core_test.c 期望值
# 3. 修复 watchdog_test.c 状态机
# 4. 运行测试验证
ctest --test-dir build --output-on-failure
```

### Step 2: 新增基础测试
```bash
# 创建新测试文件
touch tests/profile_test.c
touch tests/device_info_test.c
touch tests/transport_test.c

# 更新 CMakeLists.txt 添加新测试
# 运行测试验证
```

### Step 3: 新增高级测试
```bash
# 创建高级功能测试
touch tests/chip_detector_test.c
touch tests/usb_mode_controller_test.c

# 更新 CMakeLists.txt
# 运行测试验证
```

### Step 4: 基础设施改进
```bash
# 创建测试框架
mkdir tests/framework
touch tests/framework/test_assert.h
touch tests/framework/test_helpers.c

# 重构现有测试使用新框架
# 运行所有测试验证
```

---

## 四、验证标准

### 通过标准
- ✅ 所有现有测试通过
- ✅ 新增测试覆盖核心模块
- ✅ CI 绿色（GitHub Actions 通过）
- ✅ 代码覆盖率 > 80%

### 测试运行命令
```bash
# 本地运行所有测试
cd build && ctest --output-on-failure

# 运行特定测试
ctest --output-on-failure -R input_test
ctest --output-on-failure -R profile_test

# 详细输出
ctest --test-dir build --output-on-failure -V
```

---

## 五、时间估算

| Phase | 工作内容 | 预计时间 |
|-------|---------|---------|
| Phase 1 | 修复失败测试 | 2-4 小时 |
| Phase 2 | 新增基础测试 | 4-8 小时 |
| Phase 3 | 新增高级测试 | 4-6 小时 |
| Phase 4 | 基础设施改进 | 2-4 小时 |
| **总计** | | **12-22 小时** |

---

## 六、风险与缓解

### 风险
1. **状态机测试复杂**: watchdog 状态机涉及时间相关逻辑
   - 缓解: 使用 mock 时间源，直接设置状态测试转换

2. **硬件依赖**: HID 测试可能需要真实设备
   - 缓解: 使用 mock/stub 后端，不依赖真实硬件

3. **回归风险**: 修改测试可能引入新问题
   - 缓解: 小步提交，每次修改后运行完整测试套件

### 缓解措施
- 使用 CI 自动测试，每次推送自动验证
- 代码审查确保测试质量
- 保持测试独立性，避免测试间依赖
