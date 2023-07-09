//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: common base for windows/dialogs dealing with user-defined directories
//======================================================================================================================

#ifndef DIALOG_COMMON_INCLUDED
#define DIALOG_COMMON_INCLUDED


#include "Essential.hpp"

#include "Utils/FileSystemUtils.hpp"  // PathContext

#include <QString>

class QWidget;
class QLineEdit;


//======================================================================================================================

/// Functionality common for all dialogs and windows.
class DialogCommon {

protected:

	DialogCommon( QWidget * thisWidget );

};

/// Base for dialogs and windows dealing with user-defined directories.
class DialogWithPaths : public DialogCommon {

 protected:

	PathContext pathContext;  ///< stores path settings and automatically converts paths to relative or absolute
	QString lastUsedDir;  ///< the last directory the user selected via QFileDialog

	DialogWithPaths( QWidget * thisWidget, PathContext pathContext )
		: DialogCommon( thisWidget ), pathContext( std::move(pathContext) ) {}

	/// Runs a file-system dialog to let the user select a file and stores the its directory for the next call.
	QString browseFile( QWidget * parent, const QString & fileDesc, QString startingDir, const QString & filter );

	/// Runs a file-system dialog to let the user select a directory and stores it for the next call.
	QString browseDir( QWidget * parent, const QString & dirDesc, QString startingDir );

	/// Convenience wrapper that also stores the result into a text line.
	void browseFile( QWidget * parent, const QString & fileDesc, QLineEdit * targetLine, const QString & filter );

	/// Convenience wrapper that also stores the result into a text line.
	void browseDir( QWidget * parent, const QString & dirDesc, QLineEdit * targetLine );

};


#endif // DIALOG_COMMON_INCLUDED
