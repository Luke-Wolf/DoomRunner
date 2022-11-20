//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the New Config dialog that appears when you click the Clone Config button
//======================================================================================================================

#ifndef CONFIG_DIALOG_INCLUDED
#define CONFIG_DIALOG_INCLUDED


#include "DialogCommon.hpp"

#include <QDialog>
#include <QString>

namespace Ui {
	class NewConfigDialog;
}


//======================================================================================================================

class NewConfigDialog : public QDialog, private DialogCommon {

	Q_OBJECT

	using thisClass = NewConfigDialog;

 public:

	explicit NewConfigDialog( QWidget * parent, const QString & currentConfigName );
	virtual ~NewConfigDialog() override;

	void confirmed();

 private:

	Ui::NewConfigDialog * ui;

 public: // return values from this dialog

	QString newConfigName;

};


#endif // CONFIG_DIALOG_INCLUDED
