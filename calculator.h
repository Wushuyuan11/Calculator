#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class Calculator; }
QT_END_NAMESPACE

/**
 * @brief 带键盘事件的计算器主窗口
 *
 * 设计要点：
 * 1. 鼠标点击按钮与键盘按键共用同一套输入处理逻辑（inputDigit /
 *    inputOperator / inputDot / inputEquals / doBackspace / doClear）；
 * 2. 通过状态机管理第一操作数、运算符、第二操作数与计算结果，
 *    正确处理连续运算、连续操作符、连续小数点、除零等边界情况；
 * 3. 键盘事件由 keyPressEvent 统一分发，映射到与按钮相同的处理函数。
 */
class Calculator : public QWidget
{
    Q_OBJECT

public:
    explicit Calculator(QWidget *parent = nullptr);
    ~Calculator();

    // 应用 QSS 样式表美化界面（main 中调用，--plain 参数可跳过）
    void applyStyleSheet();

protected:
    // 重写键盘事件，实现键盘输入
    void keyPressEvent(QKeyEvent *event) override;

private:
    // ===== 统一输入处理逻辑（鼠标与键盘共用）=====
    void inputDigit(const QString &digit);      // 输入数字 0-9
    void inputOperator(const QString &op);      // 输入运算符 + - * /
    void inputDot();                            // 输入小数点
    void inputEquals();                         // 按等号计算结果
    void doBackspace();                         // 退格
    void doClear();                             // 全部清除

    // ===== 辅助函数 =====
    QString currentText() const;                // 取当前显示文本
    void updateDisplay(const QString &text);    // 更新显示框
    void updateExpression(const QString &expr); // 更新表达式标签
    QString opSymbol(const QString &op) const;  // 内部运算符(+ - * /)转界面符号(+ − × ÷)
    double displayValue() const;                // 显示文本转数值
    void setDisplayValue(double value);         // 数值转文本并显示
    double calculate(double a, double b, const QString &op, bool *ok) const;
                                                // 执行一次四则运算，ok 表示是否合法（除零为 false）
    QString formatNumber(double value) const;   // 结果格式化，避免浮点精度尾差
    void enterErrorState(const QString &msg);   // 进入错误状态（如除零）
    void connectButtons();                      // 集中连接按钮信号

private:
    Ui::Calculator *ui;

    // ===== 计算状态 =====
    double  m_firstOperand = 0.0;   // 第一个操作数
    QString m_operator;             // 当前运算符（+ - * /），空串表示尚未选择
    bool    m_waitingForSecond = false; // 是否在等待输入第二个操作数
    bool    m_justCalculated = false;   // 是否刚按过等号（结果已显示）
    bool    m_error = false;            // 是否处于错误状态（除零等）
    QString m_expression;               // 当前表达式文本（如 "12.5 + 7.8"）
};

#endif // CALCULATOR_H
