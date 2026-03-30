"""
VitaEdge Agent - OpenClaw健康守护Agent

核心机制：
1. 上下文长记忆 - 用户健康档案
2. 心跳机制 - 周期性自检
3. 情商感知 - HRV情绪识别
"""

import json
import time
from datetime import datetime, timedelta
from collections import defaultdict

# ======================== 配置 ========================

class VitaEdgeConfig:
    # 预警阈值
    HEART_RATE_MIN = 50
    HEART_RATE_MAX = 150
    BP_SYSTOLIC_MAX = 180
    BP_SYSTOLIC_MIN = 85
    GLUCOSE_MIN = 3.5
    GLUCOSE_MAX = 16.7
    
    # 心跳周期（秒）
    HEARTBEAT_INTERVAL = 300  # 5分钟
    DAILY_REPORT_HOUR = 8  # 早上8点日报
    WEEKLY_REPORT_DAY = 0  # 周一
    
    # 上下文大小
    MAX_CONTEXT_LENGTH = 1000

# ======================== 用户健康档案 ========================

class UserHealthProfile:
    """用户健康档案 - 长期记忆"""
    
    def __init__(self, user_id: str):
        self.user_id = user_id
        self.baseline_bp = (120, 80)  # 基准血压
        self.baseline_glucose = 5.8  # 基准血糖
        self.baseline_hr = 70  # 基准心率
        
        # 食物影响图谱
        self.food_impact = {}  # food -> {glucose_delta, bp_delta, duration}
        
        # 运动反应
        self.exercise_reactions = {}  # exercise -> {glucose_delta, hr_delta}
        
        # 习惯模式
        self.habit_patterns = []  # [{time, activity, effect}]
        
        # 长期趋势
        self.weekly_data = []  # 每周健康数据
        
        # 最近上下文
        self.recent_context = []
        
    def learn_food_impact(self, food: str, glucose_delta: float, hours: int):
        """学习食物对血糖的影响"""
        if food not in self.food_impact:
            self.food_impact[food] = {"count": 0, "glucose_total": 0}
        
        entry = self.food_impact[food]
        entry["count"] += 1
        entry["glucose_total"] += glucose_delta
        entry["glucose_avg"] = entry["glucose_total"] / entry["count"]
        entry["duration_hours"] = hours
        
    def learn_exercise_reaction(self, exercise: str, glucose_delta: float, hr_delta: float):
        """学习运动反应"""
        if exercise not in self.exercise_reactions:
            self.exercise_reactions[exercise] = {"count": 0, "glucose_total": 0, "hr_total": 0}
        
        entry = self.exercise_reactions[exercise]
        entry["count"] += 1
        entry["glucose_total"] += glucose_delta
        entry["hr_total"] += hr_delta
        entry["glucose_avg"] = entry["glucose_total"] / entry["count"]
        entry["hr_avg"] = entry["hr_total"] / entry["count"]
        
    def add_context(self, event: str, emotion: str = None):
        """添加上下文事件"""
        self.recent_context.append({
            "time": datetime.now().isoformat(),
            "event": event,
            "emotion": emotion
        })
        if len(self.recent_context) > self.MAX_CONTEXT_LENGTH:
            self.recent_context.pop(0)
    
    def get_context_summary(self) -> str:
        """获取上下文摘要"""
        if not self.recent_context:
            return "没有最近的上下文记录"
        
        summary = "最近发生的事：\n"
        for event in self.recent_context[-5:]:  # 最近5条
            summary += f"- {event['time'][:10]}: {event['event']}\n"
        return summary

# ======================== VitaEdge Agent ========================

class VitaEdgeAgent:
    """VitaEdge健康守护Agent"""
    
    def __init__(self, user_id: str):
        self.user_id = user_id
        self.profile = UserHealthProfile(user_id)
        self.config = VitaEdgeConfig()
        
        # 当前状态
        self.current_health = {
            "heart_rate": 70,
            "bp_systolic": 120,
            "bp_diastolic": 80,
            "glucose": 5.8,
            "hrv": 50,
            "spo2": 98,
            "stress_index": 30,
            "timestamp": None
        }
        
        # 心跳状态
        self.last_heartbeat = None
        self.heartbeat_count = 0
        
    # ======================== 核心机制1: 上下文长记忆 ========================
    
    def update_health_data(self, data: dict):
        """更新健康数据"""
        old_glucose = self.current_health.get("glucose", 5.8)
        
        self.current_health.update(data)
        self.current_health["timestamp"] = datetime.now().isoformat()
        
        # 学习食物影响
        if "food" in data:
            delta = data["glucose"] - old_glucose
            self.profile.learn_food_impact(data["food"], delta, 2)
        
        # 记录上下文
        self.record_context(data)
        
    def record_context(self, data: dict):
        """记录上下文事件"""
        # 根据异常级别记录
        anomaly_level = self.detect_anomaly()
        
        if anomaly_level >= 2:
            self.profile.add_context(
                f"健康异常: 心率{data.get('heart_rate', 0)}, 血糖{data.get('glucose', 0)}",
                emotion="concern" if anomaly_level >= 3 else "worry"
            )
        elif "food" in data:
            self.profile.add_context(f"吃了{data['food']}", emotion="normal")
        elif "exercise" in data:
            self.profile.add_context(f"运动: {data['exercise']}", emotion="happy")
    
    def get_personalized_advice(self, query: str) -> str:
        """基于上下文的个性化建议"""
        context = self.profile.get_context_summary()
        
        # 根据上下文生成个性化回复
        advice = self._generate_advice(query, context)
        return advice
    
    def _generate_advice(self, query: str, context: str) -> str:
        """生成建议"""
        # 这里应该调用LLM，但为了演示用规则
        query_lower = query.lower()
        
        if "血糖" in query or "glucose" in query_lower:
            avg_glucose = self.profile.baseline_glucose
            food_impact = self.profile.food_impact
            
            advice = f"根据你的健康档案，你的基准血糖约{avg_glucose}mmol/L。"
            
            if food_impact:
                high_impact_foods = [
                    f for f, d in food_impact.items() 
                    if d.get("glucose_avg", 0) > 0.5
                ]
                if high_impact_foods:
                    advice += f"\n我注意到你吃{', '.join(high_impact_foods[:3])}后血糖会升高较多。"
                    advice += "\n建议减少这类食物的摄入频率。"
            
            return advice
            
        elif "血压" in query or "blood pressure" in query_lower:
            bp = self.profile.baseline_bp
            advice = f"你的基准血压约{bp[0]}/{bp[1]}mmHg。"
            
            if self.current_health.get("bp_systolic", 120) > 130:
                advice += "\n最近血压偏高，建议减少盐分摄入，多运动。"
            
            return advice
            
        elif "运动" in query or "exercise" in query_lower:
            exercise_reactions = self.profile.exercise_reactions
            
            if exercise_reactions:
                advice = "根据你之前的运动数据：\n"
                for ex, data in list(exercise_reactions.items())[:3]:
                    advice += f"- {ex}: 血糖{data.get('glucose_avg', 0):+.1f}, 心率+{data.get('hr_avg', 0):.0f}\n"
            else:
                advice = "我还没有你的运动数据。开始记录吧！"
            
            return advice
        
        else:
            return f"我理解你的问题。关于{query}，让我查一下你的健康档案...\n\n{context}"
    
    # ======================== 核心机制2: 心跳机制 ========================
    
    def heartbeat(self) -> dict:
        """心跳机制 - 周期性自检"""
        self.heartbeat_count += 1
        self.last_heartbeat = datetime.now()
        
        result = {
            "heartbeat_id": self.heartbeat_count,
            "timestamp": self.last_heartbeat.isoformat(),
            "health_summary": self.get_health_summary(),
            "alerts": [],
            "actions": []
        }
        
        # 检查是否需要预警
        anomaly_level = self.detect_anomaly()
        if anomaly_level >= 2:
            result["alerts"].append(self.generate_alert(anomaly_level))
        
        # 检查是否需要主动关怀
       关怀 = self.check_proactive_care()
        if 关怀:
            result["actions"].append(关怀)
        
        # 定期生成报告
        if self.should_generate_report():
            result["actions"].append(self.generate_periodic_report())
        
        return result
    
    def detect_anomaly(self) -> int:
        """异常检测 - 返回等级 0-3"""
        health = self.current_health
        
        level = 0
        
        # 心率异常
        hr = health.get("heart_rate", 70)
        if hr < 50 or hr > 150:
            return 3
        elif hr < 55 or hr > 120:
            level = max(level, 2)
        elif hr < 60 or hr > 100:
            level = max(level, 1)
        
        # 血压异常
        bp = health.get("bp_systolic", 120)
        if bp > 180 or bp < 85:
            return 3
        elif bp > 140 or bp < 95:
            level = max(level, 2)
        elif bp > 130 or bp < 105:
            level = max(level, 1)
        
        # 血糖异常
        glucose = health.get("glucose", 5.8)
        if glucose < 3.0 or glucose > 16.7:
            return 3
        elif glucose < 3.9 or glucose > 11.0:
            level = max(level, 2)
        elif glucose < 4.4 or glucose > 8.0:
            level = max(level, 1)
        
        # HRV异常（压力大）
        hrv = health.get("hrv", 50)
        if hrv < 15:
            level = max(level, 2)
        
        return level
    
    def generate_alert(self, level: int) -> dict:
        """生成预警"""
        health = self.current_health
        
        if level == 3:
            title = "🔴 紧急预警"
            message = "检测到生命体征危急值！"
        elif level == 2:
            title = "🟠 重要提醒"
            message = "健康指标持续异常"
        else:
            title = "🟡 注意事项"
            message = "部分指标需要关注"
        
        message += f"\n\n当前数据："
        message += f"\n- 心率: {health.get('heart_rate', 0):.0f} bpm"
        message += f"\n- 血压: {health.get('bp_systolic', 0):.0f}/{health.get('bp_diastolic', 0):.0f} mmHg"
        message += f"\n- 血糖: {health.get('glucose', 0):.1f} mmol/L"
        
        return {
            "level": level,
            "title": title,
            "message": message,
            "timestamp": datetime.now().isoformat()
        }
    
    def check_proactive_care(self) -> dict:
        """主动关怀检查"""
        # 检查最近是否有异常恢复
        # 检查是否到运动时间
        # 检查睡眠时间
        
        now = datetime.now()
        hour = now.hour
        
        if hour == 8 and now.minute < 30:
            return {
                "type": "morning_greeting",
                "message": f"早安！🌅\n昨晚睡得不错（7.2小时）\n今天建议跑步30分钟，血糖会更稳定。"
            }
        
        if hour == 22:
            return {
                "type": "night_reminder",
                "message": f"该休息了！😴\n你今天运动了45分钟，完成目标！\n明天继续保持哦 💪"
            }
        
        return None
    
    def should_generate_report(self) -> bool:
        """是否生成定期报告"""
        now = datetime.now()
        
        # 每天早上8点生成日报
        if now.hour == 8 and now.minute == 0:
            return True
        
        # 每周一生成周报
        if now.weekday() == 0 and now.hour == 9 and now.minute == 0:
            return True
        
        return False
    
    def generate_periodic_report(self) -> dict:
        """生成定期报告"""
        return {
            "type": "periodic_report",
            "title": "📊 健康周报",
            "message": "本周你的健康数据表现良好，继续保持！",
            "timestamp": datetime.now().isoformat()
        }
    
    def get_health_summary(self) -> str:
        """获取健康摘要"""
        h = self.current_health
        return (
            f"心率: {h.get('heart_rate', 0):.0f} | "
            f"血压: {h.get('bp_systolic', 0):.0f}/{h.get('bp_diastolic', 0):.0f} | "
            f"血糖: {h.get('glucose', 0):.1f} | "
            f"压力: {h.get('stress_index', 0):.0f}%"
        )
    
    # ======================== 核心机制3: 情商感知 ========================
    
    def perceive_emotion(self) -> str:
        """基于HRV感知情绪"""
        hrv = self.current_health.get("hrv", 50)
        stress = self.current_health.get("stress_index", 30)
        
        if hrv > 80:
            emotion = "非常放松"
            emoji = "😌"
        elif hrv > 60:
            emotion = "状态良好"
            emoji = "🙂"
        elif hrv > 40:
            emotion = "略有压力"
            emoji = "😐"
        elif hrv > 20:
            emotion = "压力较大"
            emoji = "😰"
        else:
            emotion = "极度紧张"
            emoji = "😱"
        
        return {
            "emotion": emotion,
            "emoji": emoji,
            "hrv": hrv,
            "stress_index": stress,
            "suggestion": self._get_emotion_suggestion(emotion)
        }
    
    def _get_emotion_suggestion(self, emotion: str) -> str:
        """根据情绪给出建议"""
        suggestions = {
            "非常放松": "继续保持！你的身心状态很好 🌟",
            "状态良好": "状态不错！可以适当挑战一些事情 😊",
            "略有压力": "建议休息一下，深呼吸3分钟 🧘",
            "压力较大": "压力有点大，建议做些放松的活动",
            "极度紧张": "你现在非常紧张，请立即停止工作，休息一下 😔"
        }
        return suggestions.get(emotion, "")

# ======================== 示例使用 ========================

if __name__ == "__main__":
    # 创建Agent
    agent = VitaEdgeAgent("user_001")
    
    # 更新健康数据
    agent.update_health_data({
        "heart_rate": 75,
        "bp_systolic": 128,
        "bp_diastolic": 82,
        "glucose": 6.2,
        "hrv": 45,
        "stress_index": 40
    })
    
    # 查询建议
    print("=== 查询血糖建议 ===")
    print(agent.get_personalized_advice("我的血糖怎么样？"))
    
    print("\n=== 情绪感知 ===")
    emotion = agent.perceive_emotion()
    print(f"当前情绪: {emotion['emoji']} {emotion['emotion']}")
    print(f"建议: {emotion['suggestion']}")
    
    print("\n=== 心跳检测 ===")
    result = agent.heartbeat()
    print(f"健康状态: {result['health_summary']}")
    if result['alerts']:
        print(f"预警: {result['alerts'][0]['title']}")
