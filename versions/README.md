# Versions Index

该目录用于沉淀 VitaEdge 各版本快照，便于按版本回看和对比迭代。

## 版本列表

- `v2`（2026-03-31）
  - 路径：`versions/v2/`
  - 主要内容：
    - `README.md`：升级版项目说明（动机、架构、快速开始、BOM）
    - `agent/vitaedge_agent.py`：Agent 迭代版核心逻辑
    - `docs/architecture.md`：系统技术架构说明

## 与主干关系

- 主干目录（根目录下 `README.md`、`agent/`、`docs/`）用于持续开发。
- `versions/` 目录用于固定版本快照，不随日常小改动频繁变化。
- 发布新版本时，建议新增 `versions/vX/` 并在 `CHANGELOG.md` 记录差异。

## 后续建议

- 若需补 `v1`，可从首个可用稳定提交回填一个 `versions/v1/` 快照。
- 若需正式发布，可在 GitHub 创建 Release，并附带对应版本压缩包。
