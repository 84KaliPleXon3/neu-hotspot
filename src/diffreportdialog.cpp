#include "diffreportdialog.h"
#include "ui_diffreportdialog.h"

DiffReportDialog::DiffReportDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DiffReportDialog)
{
    ui->setupUi(this);
    const auto filterString = QLatin1String("perf*.data\nperf.data*");
    ui->file_a->setFilter(filterString);
    ui->file_b->setFilter(filterString);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
}

DiffReportDialog::~DiffReportDialog() = default;

QString DiffReportDialog::fileA() const
{
    return ui->file_a->url().path();
}

QString DiffReportDialog::fileB() const
{
    return ui->file_b->url().path();
}
