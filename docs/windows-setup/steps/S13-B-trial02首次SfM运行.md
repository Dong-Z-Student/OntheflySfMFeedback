# S13-B：trial02首次SfM运行

当前：已通过。2026-10-14 21:50–21:52完成trial02首次运行；128张输入全部注册，得到31039个三维点，相机与点云显示正常。当前进入S14显示验收。

## 本轮固定输入

* 输入E:\sfm\data\trial02\images，128张South Building JPG，3072x2304，总233850173字节。
* 逐文件SHA256与E:\ColmapProject\south-building\images一致；清单SHA256为5183A51BCA95F895C68491E1C27BD9D5ED644260B27AF8227F98150B095E8855。
* 结果目录E:\sfm\data\trial02\result已存在且当前为空，trial02.db尚不存在。
* INI沿用S12：InitialImagesNum=20，SIMPLE_RADIAL，useFocalLength=false/useFactor=0.704，Feedback StartCreateMesh=40/Step=10，AutoExportModel=false。本轮先观察SfM，不提前调整参数。

## 1. 新建GUI项目

1. 在已打开的主窗口选择菜单Project → New Project。
2. Project对话框Database一行选择New，在文件选择对话框指定E:\sfm\data\trial02\result\trial02.db。
3. Images一行选择Select，指定E:\sfm\data\trial02\images。
4. 核对两行路径，点击Project对话框的Save。

通过标准：Project对话框关闭，没有Invalid paths；Project → Export Project从灰色变为可用。源码Save仅设置图像目录及导出数据库路径，数据库将在导出时写入；不以此刻出现.db文件作为建项目通过条件。

若路径错误，在尚未启动输入时重新打开New Project修改；若已经开始处理，不重复建项目或复用数据库对象。

## 2. 开始本地照片回放

1. 确认Track Model勾选（源码默认勾选），以便重建更新后刷新Model Viewer。
2. 选择Transmit → Start Receive一次。
3. 观察Image Preview/Log/Model Viewer，以及下方Images/Points状态。

Start Receive源码同时启动接收及本地照片回放。检验：Start Receive变灰、Stop Receive可用，影像预览出现照片，日志开始输出照片处理内容。前序处理累积后才会尝试初始化；InitialImagesNum=20，所以刚开始0 Points并不表示失败。

首次运行保持一次回放，不重复点击Start Receive或Stop Receive再Start。Stop Receive不能当作所有后台任务已取消的证据；若出错先记录错误和状态，再定位。

## 3. 检验SfM和增量注册

寻找日志：

```text
Initialization successful!
image ... registered successfully!
```

最低阶段证据：初始化成功，随后还有影像注册成功；Model Viewer出现相机和非空点云；底部Images大于2、Points大于0，随注册继续更新。源码此处Images是模型已注册影像数，Points是三维点数。图片预览增加仅证明读入，不能代替重建验收。

等待整组数据回放及后台队列继续处理。源照片128张不等于最终应注册128张；记录实际注册数和失败/未注册日志，不预设结果。反馈计算可能在注册数达到既有阈值后自动调度，S14另验收网格/质量/路径显示。

## 4. 返回本轮证据

现有UI是S12B retry1启动的，继续写入同一stdout/stderr文件，无须关闭重开：

```powershell
Get-Content -LiteralPath 'E:\sfm\setup\S12B-retry1-UI-stderr.log' -Tail 80
Get-Content -LiteralPath 'E:\sfm\setup\S12B-retry1-UI-stdout.log' -Tail 40
```

返回新项目Save是否成功、是否看到点云/相机、底部Images与Points数，以及日志末尾。可附主窗口截图辅助确认显示。若程序退出或报错，保留第一条真实错误及上下文，不先更换数据或重新配置全部环境。

本轮数据库/模型导出在S15执行。S13-B的最终证据为：截图显示128 Images、31039 Points及相机/点云；日志含128条全局特征、1次初始化、126条增量注册和最终渲染。8条非空路径规划说明Feedback计算已经执行，但GUI显示另由S14逐项验收。

证据快照：

* 运行日志（本机归档，见相邻步骤说明）
* 主窗口截图（本机归档，见相邻步骤说明）
