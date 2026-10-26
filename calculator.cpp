#include "calculator.h"
#include "ui_calculator.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <cmath>

Calculator::Calculator(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Calculator)
{
    ui->setupUi(this);

    setWindowTitle("带键盘事件的计算器");

    // 所有按钮和显示框均不抢占键盘焦点，
    // 保证按键事件总能由本窗口统一接收处理（键盘输入的可靠性保证）
    setFocusPolicy(Qt::StrongFocus);
    for (QPushButton *btn : findChildren<QPushButton *>()) {
        btn->setFocusPolicy(Qt::NoFocus);
    }
    ui->displayEdit->setFocusPolicy(Qt::NoFocus);

    connectButtons();
}

Calculator::~Calculator()
{
    delete ui;
}

// ------------------------------------------------------------------
// 按钮信号集中连接：鼠标点击的入口，内部统一转调输入处理逻辑
// ------------------------------------------------------------------
void Calculator::connectButtons()
{
    // 数字按钮 0-9：用对象名 btn0...btn9 统一查找、统一连接，
    // 避免 10 段重复代码
    for (int i = 0; i <= 9; ++i) {
        QPushButton *btn = findChild<QPushButton *>(QStringLiteral("btn%1").arg(i));
        if (btn) {
            connect(btn, &QPushButton::clicked, this, [this, i] {
                inputDigit(QString::number(i));
            });
        }
    }

    // 运算符按钮：对象名 -> 内部运算符 映射表
    const struct { const char *name; QString op; } opMap[] = {
        {"btnAdd",      QStringLiteral("+")},
        {"btnSubtract", QStringLiteral("-")},
        {"btnMultiply", QStringLiteral("*")},
        {"btnDivide",   QStringLiteral("/")}
    };
    for (const auto &e : opMap) {
        QPushButton *btn = findChild<QPushButton *>(QLatin1String(e.name));
        if (btn) {
            connect(btn, &QPushButton::clicked, this, [this, e] {
                inputOperator(e.op);
            });
        }
    }

    connect(ui->btnDot,     &QPushButton::clicked, this, [this] { inputDot(); });
    connect(ui->btnEquals,  &QPushButton::clicked, this, [this] { inputEquals(); });
    connect(ui->btnBackspace, &QPushButton::clicked, this, [this] { doBackspace(); });
    connect(ui->btnClear,   &QPushButton::clicked, this, [this] { doClear(); });
}

// ------------------------------------------------------------------
// 键盘事件处理：将物理按键映射到与鼠标按钮相同的处理逻辑
// ------------------------------------------------------------------
void Calculator::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    // 数字键：主键盘和小键盘（0-9）
    case Qt::Key_0: case Qt::Key_1: case Qt::Key_2:
    case Qt::Key_3: case Qt::Key_4: case Qt::Key_5:
    case Qt::Key_6: case Qt::Key_7: case Qt::Key_8:
    case Qt::Key_9:
        inputDigit(QString::number(event->key() - Qt::Key_0));
        break;

    // 小数点（主键盘 . 和小键盘 . 在 NumLock 开启时同样映射为 Key_Period）
    case Qt::Key_Period:
        inputDot();
        break;

    // 运算符：主键盘 * / 与小键盘 * /（NumLock 开启时同为 Key_Asterisk / Key_Slash）
    case Qt::Key_Plus:
        inputOperator(QStringLiteral("+"));
        break;
    case Qt::Key_Minus:
        inputOperator(QStringLiteral("-"));
        break;
    case Qt::Key_Asterisk:
    case Qt::Key_multiply:
        inputOperator(QStringLiteral("*"));
        break;
    case Qt::Key_Slash:
        inputOperator(QStringLiteral("/"));
        break;

    // 等号 / 回车：计算结果
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Equal:
        inputEquals();
        break;

    // 退格
    case Qt::Key_Backspace:
        doBackspace();
        break;

    // 清除：Esc、Delete 或单独按 C（带 Ctrl 时保留系统习惯，不拦截）
    case Qt::Key_Escape:
    case Qt::Key_Delete:
        doClear();
        break;
    case Qt::Key_C:
        if (!event->modifiers().testFlag(Qt::ControlModifier))
            doClear();
        break;

    default:
        QWidget::keyPressEvent(event);
        return;
    }
    event->accept();
}

// ------------------------------------------------------------------
// 统一输入处理逻辑（鼠标与键盘共用）
// ------------------------------------------------------------------

// 数字输入
void Calculator::inputDigit(const QString &digit)
{
    if (m_error)
        return;   // 错误状态下数字键无效，必须先按 C 清除

    QString text = currentText();

    if (m_waitingForSecond || m_justCalculated) {
        // 刚选过运算符（等待第二操作数）或刚出结果：开始输入一个新的操作数
        text = digit;
        m_waitingForSecond = false;
        m_justCalculated = false;
    } else if (text == QStringLiteral("0")) {
        // 显示为 0 时直接替换，避免出现 "07" 这类前导零
        text = digit;
    } else {
        // 正常追加数字
        text += digit;
    }
    updateDisplay(text);

    // 正在输入第二个操作数时，同步更新表达式标签（如 "12.5 + 7"）
    if (!m_operator.isEmpty() && !m_waitingForSecond) {
        updateExpression(QStringLiteral("%1 %2 %3")
                             .arg(formatNumber(m_firstOperand), opSymbol(m_operator), text));
    }
}

// 运算符输入
void Calculator::inputOperator(const QString &op)
{
    if (m_error)
        return;

    QString text = currentText();

    if (m_waitingForSecond) {
        // 连续操作符（如 1 + + 2）：直接用新运算符覆盖旧运算符
        m_operator = op;
        updateExpression(QStringLiteral("%1 %2 ")
                             .arg(formatNumber(m_firstOperand), opSymbol(op)));
        return;
    }

    if (m_justCalculated) {
        // 计算完成后按运算符：把结果显示作为第一个操作数继续运算（如 2+3=5 后按 +）
        m_firstOperand = displayValue();
        m_operator = op;
        m_waitingForSecond = true;
        m_justCalculated = false;
        updateExpression(QStringLiteral("%1 %2 ")
                             .arg(formatNumber(m_firstOperand), opSymbol(op)));
        return;
    }

    if (m_operator.isEmpty()) {
        // 首次选择运算符：保存当前显示为第一个操作数
        m_firstOperand = displayValue();
    } else {
        // 已存在运算符且又输入了新的运算符：先完成前一次运算，实现连续运算（如 1+2+3）
        bool ok = false;
        double result = calculate(m_firstOperand, displayValue(), m_operator, &ok);
        if (!ok) {
            enterErrorState(QStringLiteral("错误：除数不能为 0"));
            return;
        }
        m_firstOperand = result;
        setDisplayValue(result);
    }

    m_operator = op;
    m_waitingForSecond = true;
    // 表达式显示为 "第一个操作数 运算符"（等待输入第二个操作数）
    updateExpression(QStringLiteral("%1 %2 ")
                         .arg(formatNumber(m_firstOperand), opSymbol(op)));
}

// 小数点输入
void Calculator::inputDot()
{
    if (m_error)
        return;

    if (m_waitingForSecond || m_justCalculated) {
        // 新操作数从 "0." 开始
        updateDisplay(QStringLiteral("0."));
        m_waitingForSecond = false;
        m_justCalculated = false;
        if (!m_operator.isEmpty()) {
            updateExpression(QStringLiteral("%1 %2 0.")
                                 .arg(formatNumber(m_firstOperand), opSymbol(m_operator)));
        }
        return;
    }

    QString text = currentText();
    if (text.contains(QLatin1Char('.')))
        return;   // 关键：同一操作数只允许一个小数点，重复输入直接忽略

    text += QLatin1Char('.');
    updateDisplay(text);
    if (!m_operator.isEmpty()) {
        updateExpression(QStringLiteral("%1 %2 %3")
                             .arg(formatNumber(m_firstOperand), opSymbol(m_operator), text));
    }
}

// 等号：计算并显示结果
void Calculator::inputEquals()
{
    if (m_error)
        return;
    if (m_operator.isEmpty())
        return;   // 没有运算符时等号无效

    double second = displayValue();
    bool ok = false;
    double result = calculate(m_firstOperand, second, m_operator, &ok);
    if (!ok) {
        enterErrorState(QStringLiteral("错误：除数不能为 0"));
        return;
    }

    // 表达式标签显示完整的计算式子，主显示框显示结果
    updateExpression(QStringLiteral("%1 %2 %3 =")
                         .arg(formatNumber(m_firstOperand), opSymbol(m_operator),
                              formatNumber(second)));
    setDisplayValue(result);
    m_operator.clear();        // 清除运算符，进入"结果状态"
    m_waitingForSecond = false;
    m_justCalculated = true;   // 之后按数字开始新计算，按运算符用结果继续算
}

// 退格
void Calculator::doBackspace()
{
    if (m_error)
        return;
    // 等待第二操作数或刚出结果时退格无意义，直接忽略
    if (m_waitingForSecond || m_justCalculated)
        return;

    QString text = currentText();
    if (text.length() <= 1) {
        // 只剩一位（0 或 0.）时退格回到 0
        text = QStringLiteral("0");
    } else {
        text.chop(1);
    }
    updateDisplay(text);
    // 输入第二个操作数过程中退格，同步更新表达式
    if (!m_operator.isEmpty()) {
        updateExpression(QStringLiteral("%1 %2 %3")
                             .arg(formatNumber(m_firstOperand), opSymbol(m_operator), text));
    }
}

// 全部清除：重置所有状态，回到初始显示 0
void Calculator::doClear()
{
    m_firstOperand = 0.0;
    m_operator.clear();
    m_waitingForSecond = false;
    m_justCalculated = false;
    m_error = false;
    updateExpression(QString());
    updateDisplay(QStringLiteral("0"));
}

// ------------------------------------------------------------------
// 辅助函数
// ------------------------------------------------------------------

QString Calculator::currentText() const
{
    return ui->displayEdit->text();
}

void Calculator::updateDisplay(const QString &text)
{
    ui->displayEdit->setText(text);
}

void Calculator::updateExpression(const QString &expr)
{
    m_expression = expr;
    ui->expressionLabel->setText(expr);
}

// 内部运算符转界面符号（+ - * / -> + − × ÷）
QString Calculator::opSymbol(const QString &op) const
{
    if (op == QStringLiteral("+")) return QStringLiteral("+");
    if (op == QStringLiteral("-")) return QStringLiteral("−");
    if (op == QStringLiteral("*")) return QStringLiteral("×");
    if (op == QStringLiteral("/")) return QStringLiteral("÷");
    return QString();
}

double Calculator::displayValue() const
{
    return currentText().toDouble();
}

void Calculator::setDisplayValue(double value)
{
    updateDisplay(formatNumber(value));
}

// 四则运算；除数为 0 时返回 false，不产生结果
double Calculator::calculate(double a, double b, const QString &op, bool *ok) const
{
    *ok = true;
    if (op == QStringLiteral("+")) return a + b;
    if (op == QStringLiteral("-")) return a - b;
    if (op == QStringLiteral("*")) return a * b;
    if (op == QStringLiteral("/")) {
        if (b == 0.0) {           // 除零保护
            *ok = false;
            return 0.0;
        }
        return a / b;
    }
    *ok = false;
    return 0.0;
}

// 结果格式化：使用 'g' 格式（最多 12 位有效数字），
// 避免 0.1+0.2 显示成 0.30000000000000004 之类的浮点尾差
QString Calculator::formatNumber(double value) const
{
    if (std::isnan(value) || std::isinf(value))
        return QStringLiteral("错误");
    return QString::number(value, 'g', 12);
}

// 进入错误状态：显示错误信息，之后只能按 C 恢复
void Calculator::enterErrorState(const QString &msg)
{
    m_error = true;
    m_operator.clear();
    m_waitingForSecond = false;
    m_justCalculated = false;
    updateExpression(QString());
    updateDisplay(msg);
}

// ------------------------------------------------------------------
// QSS 样式表：改善界面视觉效果（深色主题 + 橙色等号键）
// ------------------------------------------------------------------
void Calculator::applyStyleSheet()
{
    setStyleSheet(QStringLiteral(R"(
        QWidget#Calculator {
            background-color: #2b2b2b;
        }
        QLineEdit#displayEdit {
            background-color: #1e1e1e;
            color: #ffffff;
            font-size: 36px;
            font-weight: bold;
            border: 1px solid #444444;
            border-radius: 8px;
            padding: 10px 14px;
        }
        QLabel#expressionLabel {
            color: #a8a8a8;
            font-size: 14px;
            background: transparent;
        }
        QPushButton {
            background-color: #3c3c3c;
            color: #ffffff;
            font-size: 20px;
            border: none;
            border-radius: 8px;
        }
        QPushButton:hover {
            background-color: #4d4d4d;
        }
        QPushButton:pressed {
            background-color: #5d5d5d;
        }
        QPushButton#btnClear, QPushButton#btnBackspace {
            background-color: #5c5c5c;
        }
        QPushButton#btnEquals {
            background-color: #ff9500;
            color: #ffffff;
            font-size: 24px;
        }
        QPushButton#btnEquals:hover {
            background-color: #ffaa33;
        }
        QPushButton#btnEquals:pressed {
            background-color: #e08600;
        }
    )"));
}
