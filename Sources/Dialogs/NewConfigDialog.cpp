//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the New Config dialog that appears when you click the Clone Config button
//======================================================================================================================

#include "NewConfigDialog.hpp"
#include "ui_NewConfigDialog.h"


//======================================================================================================================

NewConfigDialog::NewConfigDialog( QWidget * parent, const QString & currentConfigName )
:
	QDialog( parent ),
	DialogCommon( this )
{
	ui = new Ui::NewConfigDialog;
	ui->setupUi(this);

	ui->configNameLine->setText( currentConfigName );

	connect( this, &QDialog::accepted, this, &thisClass::confirmed );
}

void NewConfigDialog::confirmed()
{
	newConfigName = ui->configNameLine->text();
}

NewConfigDialog::~NewConfigDialog()
{
	delete ui;
}
