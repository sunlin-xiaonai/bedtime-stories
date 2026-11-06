# 晚安故事（开发中）

TikPal Voice 睡前故事应用，面向 3–6 岁儿童。目标是选择故事播放，并可设置定时停止。优先寻找可免费使用的普通话录音，逐篇记录版权和署名。

## 当前状态

本仓库处于研究与原型阶段，尚不是可用的故事播放器。`main/` 仍是开发包生成的计数器模板，不代表产品功能；不要刷入设备或提交上架。`prototype/` 是独立的定时停止逻辑，尚未连接音频或界面。运行 `./prototype/run-tests.sh` 做主机测试。

## 资源候选

Storybooks Canada 提供普通话录音。其音频按故事分别采用 CC BY 或 CC BY-NC；项目优先筛选允许商业使用的 CC BY，逐篇检查并保留署名及许可。以下仅为候选，尚未完成录音试听、适龄审查或导入：

- 《和植物聊天的卡莱》：https://www.storybookscanada.ca/stories/zh/0089/ ，CC BY 4.0；作者 Ursula Nafula，译者 Vicky Liu，朗读 Zhuo Sun。
- 《我喜欢看书》：https://www.storybookscanada.ca/stories/zh/0087/ ，CC BY 3.0；作者 Letta Machoga，译者 dohliam，朗读 Zhuo Sun。

来源项目说明录音的许可证随原故事而异：https://github.com/global-asp/gsn-audio 。内容是否适合睡前及应用分发的最终署名形式仍需检查。避免直接依赖第三方站点作为生产音频服务，需先确认托管、访问稳定性和平台规则。

## 开发约束

开发包 `devkit/` 是只读子模块。设备为 ESP32-S3，466×466 圆屏，支持扬声器及 TF 卡；扩展固件分区上限 3.5 MB。云能力需在 TikPal 立项获回复并开通后开发；不得擅自刷机、发布、产生费用或写死服务器及密钥。应用编号、在线资源架构、播放/暂停时定时器行为、音频来源及实际设备测试均待确定。
