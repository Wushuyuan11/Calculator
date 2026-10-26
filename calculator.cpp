#include "calculator.h"
#include "ui_calculator.h"

#include <QKeyEvent>
#include <QPushButton>

// ===== V5：实现键盘事件，鼠标与键盘复用同一套输入处理逻辑 =====
// keyPressEvent 把物理按键映射到 inputDigit / inputOperator / inputDot /
// inputEquals / doBackspace / doClear，与按钮点击走完全相同的代码路径。

Calculator::Calculator(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Calculator)
{
    ui->setupUi(this);
    setWindowTitle("带键盘事件的计算器");
    connectButtons();
}

Calculator::~Calculator()
{
    delete ui;
}

void Calculator::applyStyleSheet() {}

void Calculator::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_0: case Qt::Key_1: case Qt::Key_2:
    case Qt::Key_3: case Qt::Key_4: case Qt::Key_5:
    case Qt::Key_6: case Qt::Key_7: case Qt::Key_8:
    case Qt::Key_9:
        inputDigit(QString::number(event->key() - Qt::Key_0));
        break;
    case Qt::Key_Period:
        inputDot();
        break;
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
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Equal:
        inputEquals();
        break;
    case Qt::Key_Backspace:
        doBackspace();
        break;
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

void Calculator::connectButtons()
{
    for (int i = 0; i <= 9; ++i) {
        QPushButton *btn = findChild<QPushButton *>(QStringLiteral("btn%1").arg(i));
        if (btn) {
            connect(btn, &QPushButton::clicked, this, [this, i] {
                inputDigit(QString::number(i));
            });
        }
    }
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

void Calculator::inputDigit(const QString &digit)
{
    if (m_error) return;
    QString text = currentText();
    if (m_waitingForSecond) {
        text = digit;
        m_waitingForSecond = false;
    } else if (text == QStringLiteral("0")) {
        text = digit;
    } else {
        text += digit;
    }
    updateDisplay(text);
}

void Calculator::inputOperator(const QString &op)
{
    if (m_error) return;
    if (m_waitingForSecond) {
        m_operator = op;
        return;
    }
    if (m_operator.isEmpty()) {
        m_firstOperand = displayValue();
    } else {
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
}

void Calculator::inputDot()
{
    if (m_error) return;
    if (m_waitingForSecond) {
        updateDisplay(QStringLiteral("0."));
        m_waitingForSecond = false;
        return;
    }
    QString text = currentText();
    if (text.contains(QLatin1Char('.')))
        return;
    updateDisplay(text + QLatin1Char('.'));
}

void Calculator::inputEquals()
{
    if (m_error) return;
    if (m_operator.isEmpty()) return;
    bool ok = false;
    double result = calculate(m_firstOperand, displayValue(), m_operator, &ok);
    if (!ok) {
        enterErrorState(QStringLiteral("错误：除数不能为 0"));
        return;
    }
    setDisplayValue(result);
    m_operator.clear();
    m_waitingForSecond = false;
}

void Calculator::doBackspace()
{
    if (m_error || m_waitingForSecond) return;
    QString text = currentText();
    text.chop(1);
    updateDisplay(text.isEmpty() ? QStringLiteral("0") : text);
}

void Calculator::doClear()
{
    m_firstOperand = 0.0;
    m_operator.clear();
    m_waitingForSecond = false;
    m_error = false;
    updateDisplay(QStringLiteral("0"));
}

QString Calculator::currentText() const { return ui->displayEdit->text(); }
void Calculator::updateDisplay(const QString &text) { ui->displayEdit->setText(text); }
void Calculator::updateExpression(const QString &expr) { Q_UNUSED(expr); }
QString Calculator::opSymbol(const QString &op) const { Q_UNUSED(op); return QString(); }
double Calculator::displayValue() const { return currentText().toDouble(); }
void Calculator::setDisplayValue(double value) { updateDisplay(formatNumber(value)); }
double Calculator::calculate(double a, double b, const QString &op, bool *ok) const
{
    *ok = true;
    if (op == QStringLiteral("+")) return a + b;
    if (op == QStringLiteral("-")) return a - b;
    if (op == QStringLiteral("*")) return a * b;
    if (op == QStringLiteral("/")) {
        if (b == 0.0) {
            *ok = false;
            return 0.0;
        }
        return a / b;
    }
    *ok = false;
    return 0.0;
}
QString Calculator::formatNumber(double value) const
{
    return QString::number(value, 'g', 12);
}
void Calculator::enterErrorState(const QString &msg)
{
    m_error = true;
    m_operator.clear();
    m_waitingForSecond = false;
    updateDisplay(msg);
}
