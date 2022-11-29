//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Qt widget helpers
//======================================================================================================================

#ifndef WIDGET_UTILS_INCLUDED
#define WIDGET_UTILS_INCLUDED


#include "Common.hpp"

#include "LangUtils.hpp"         // findSuch
#include "FileSystemUtils.hpp"   // fillListFromDir
#include "Widgets/ListModel.hpp"

#include <QAbstractItemView>
#include <QListView>
#include <QTreeView>
#include <QComboBox>
#include <QScrollBar>
#include <QColor>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <functional>
#include <type_traits>


//======================================================================================================================
// implementation notes
//
// When an item is in edit mode and current index changes, the content of the line editor is dumped into the old
// current item and the edit mode closed. So if you make any changes to the order of the items and then change
// the current item, the editor content gets saved into a wrong item.
// So before any re-ordering, the current item is unset (set to invalid QModelIndex) to force the content dump before
// the reordering, and then set the current item to the new one, after the reordering is done.




//======================================================================================================================
//  selection manipulation


//----------------------------------------------------------------------------------------------------------------------
//  list view helpers

// current item
int getCurrentItemIndex( QListView * view );
void setCurrentItemByIndex( QListView * view, int index );
void unsetCurrentItem( QListView * view );

// selected items
bool isSelectedIndex( QListView * view, int index );
bool isSomethingSelected( QListView * view );
int getSelectedItemIndex( QListView * view );  // assumes a single-selection mode, will throw a message box error otherwise
QVector<int> getSelectedItemIndexes( QListView * view );
void selectItemByIndex( QListView * view, int index );
void deselectItemByIndex( QListView * view, int index );
void deselectSelectedItems( QListView * view );

// high-level control
void selectAndSetCurrentByIndex( QListView * view, int index );
void deselectAllAndUnsetCurrent( QListView * view );
/// Deselects currently selected items, selects new one and makes it the current item.
/** Basically equivalent to left-clicking on an item. */
void chooseItemByIndex( QListView * view, int index );


//----------------------------------------------------------------------------------------------------------------------
//  tree view helpers

// current item
QModelIndex getCurrentItemIndex( QTreeView * view );
void setCurrentItemByIndex( QTreeView * view, const QModelIndex & index );
void unsetCurrentItem( QTreeView * view );

// selected items
bool isSelectedIndex( QTreeView * view, const QModelIndex & index );
bool isSomethingSelected( QTreeView * view );
QModelIndex getSelectedItemIndex( QTreeView * view );
QModelIndexList getSelectedItemIndexes( QTreeView * view );
QModelIndexList getSelectedRows( QTreeView * view );
void selectItemByIndex( QTreeView * view, const QModelIndex & index );
void deselectItemByIndex( QTreeView * view, const QModelIndex & index );
void deselectSelectedItems( QTreeView * view );

// high-level control
void selectAndSetCurrentByIndex( QTreeView * view, const QModelIndex & index );
void deselectAllAndUnsetCurrent( QTreeView * view );
/// Deselects currently selected items, selects new one and makes it the current item.
/** Basically equivalent to left-clicking on an item. */
void chooseItemByIndex( QTreeView * view, const QModelIndex & index );




//======================================================================================================================
//  button actions - all of these function assume a 1-dimensional non-recursive list view/widget


/// Adds an item to the end of the list and selects it.
template< typename Item >
void appendItem( QListView * view, AListModel< Item > & model, const Item & item )
{
	deselectAllAndUnsetCurrent( view );

	model.startAppending( 1 );

	model.append( item );

	model.finishAppending();

	selectAndSetCurrentByIndex( view, model.size() - 1 );
}

/// Adds an item to the begining of the list and selects it.
template< typename Item >
void prependItem( QListView * view, AListModel< Item > & model, const Item & item )
{
	deselectAllAndUnsetCurrent( view );

	model.startInserting( 0 );

	model.prepend( item );

	model.finishInserting();

	selectAndSetCurrentByIndex( view, 0 );
}

/// Adds an item to the middle of the list and selects it.
template< typename Item >
void insertItem( QListView * view, AListModel< Item > & model, const Item & item, int index )
{
	deselectAllAndUnsetCurrent( view );

	model.startInserting( index );

	model.insert( index, item );

	model.finishInserting();

	selectAndSetCurrentByIndex( view, index );
}

/// Deletes a selected item and attempts to select an item following the deleted one.
/** Pops up a warning box if nothing is selected. */
template< typename Item >
int deleteSelectedItem( QListView * view, AListModel< Item > & model )
{
	int selectedIdx = getSelectedItemIndex( view );
	if (selectedIdx < 0)
	{
		if (!model.isEmpty())
			QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	deselectAllAndUnsetCurrent( view );

	model.startDeleting( selectedIdx );

	model.removeAt( selectedIdx );

	model.finishDeleting();

	// try to select some nearest item, so that user can click 'delete' repeatedly to delete all of them
	if (selectedIdx < model.size())
	{
		selectAndSetCurrentByIndex( view, selectedIdx );
	}
	else  // ........................................................... if the deleted item was the last one,
	{
		if (selectedIdx > 0)  // ....................................... and not the only one,
		{
			selectAndSetCurrentByIndex( view, selectedIdx - 1 );  // ... select the previous
		}
	}

	return selectedIdx;
}

/// Deletes all selected items and attempts to select the item following the deleted ones.
/** Pops up a warning box if nothing is selected. */
template< typename Item >
QVector<int> deleteSelectedItems( QListView * view, AListModel< Item > & model )
{
	QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty())
	{
		if (!model.isEmpty())
			QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return {};
	}

	// the list of indexes is not sorted, they are in the order in which user selected them
	// but for the delete, we need them sorted in ascending order
	QVector<int> selectedIndexesAsc;
	for (const QModelIndex & index : selectedIndexes)
		selectedIndexesAsc.push_back( index.row() );
	std::sort( selectedIndexesAsc.begin(), selectedIndexesAsc.end(), []( int idx1, int idx2 ) { return idx1 < idx2; } );

	int firstSelectedIdx = selectedIndexesAsc[0];

	deselectAllAndUnsetCurrent( view );

	model.startCompleteUpdate();

	// delete all the selected items
	uint deletedCnt = 0;
	for (int selectedIdx : selectedIndexesAsc)
	{
		model.removeAt( selectedIdx - deletedCnt );
		deletedCnt++;
	}

	model.finishCompleteUpdate();

	// try to select some nearest item, so that user can click 'delete' repeatedly to delete all of them
	if (firstSelectedIdx < model.size())                       // if the first deleted item index is still within range of existing ones,
	{
		selectAndSetCurrentByIndex( view, firstSelectedIdx );  // select that one,
	}
	else if (!model.isEmpty())                                 // otherwise select the previous, if there is any
	{
		selectAndSetCurrentByIndex( view, firstSelectedIdx - 1 );
	}

	return selectedIndexesAsc;
}

/// Creates a copy of a selected item and selects the newly created one.
/** Pops up a warning box if nothing is selected. */
template< typename Item >
int cloneSelectedItem( QListView * view, AListModel< Item > & model )
{
	int selectedIdx = getSelectedItemIndex( view );
	if (selectedIdx < 0)
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	deselectAllAndUnsetCurrent( view );

	model.startAppending( 1 );

	model.append( model[ selectedIdx ] );

	model.finishAppending();

	// append some postfix to the item name to distinguish it from the original
	QModelIndex newItemIdx = model.index( model.size() - 1, 0 );
	QString origName = model.data( newItemIdx, Qt::EditRole ).toString();
	model.setData( newItemIdx, origName+" - clone", Qt::EditRole );

	model.contentChanged( newItemIdx.row() );

	selectAndSetCurrentByIndex( view, model.size() - 1 );

	return selectedIdx;
}

/// Moves a selected item up and updates the selection to point to the new position.
/** Pops up a warning box if nothing is selected. */
template< typename Item >
int moveUpSelectedItem( QListView * view, AListModel< Item > & model )
{
	int selectedIdx = getSelectedItemIndex( view );
	if (selectedIdx < 0)
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	// if the selected item is the first one, do nothing
	if (selectedIdx == 0)
	{
		return selectedIdx;
	}

	int currentIdx = getCurrentItemIndex( view );

	unsetCurrentItem( view );
	deselectItemByIndex( view, selectedIdx );

	model.orderAboutToChange();

	model.move( selectedIdx, selectedIdx - 1 );

	model.orderChanged();

	selectAndSetCurrentByIndex( view, selectedIdx - 1 );
	if (currentIdx >= 1)                                // if the current item was not the first one,
	{
		setCurrentItemByIndex( view, currentIdx - 1 );  // set the previous one as current,
	}
	else                                                // otherwise
	{
		setCurrentItemByIndex( view, 0 );               // set the first one as current
	}

	return selectedIdx;
}

/// Moves a selected item down and updates the selection to point to the new position.
/** Pops up a warning box if nothing is selected. */
template< typename Item >
int moveDownSelectedItem( QListView * view, AListModel< Item > & model )
{
	int selectedIdx = getSelectedItemIndex( view );
	if (selectedIdx < 0)
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	// if the selected item is the last one, do nothing
	if (selectedIdx == model.size() - 1)
	{
		return selectedIdx;
	}

	int currentIdx = getCurrentItemIndex( view );

	unsetCurrentItem( view );
	deselectItemByIndex( view, selectedIdx );

	model.orderAboutToChange();

	model.move( selectedIdx, selectedIdx + 1 );

	model.orderChanged();

	selectAndSetCurrentByIndex( view, selectedIdx + 1 );
	if (currentIdx < model.size() - 1)                    // if the current item was not the last one,
	{
		setCurrentItemByIndex( view, currentIdx + 1 );    // set the next one as current,
	}
	else                                                  // otherwise
	{
		setCurrentItemByIndex( view, model.size() - 1 );  // set the last one as current
	}

	return selectedIdx;
}

/// Moves all selected items up and updates the selection to point to the new position.
/** Pops up a warning box if nothing is selected. */
template< typename Item >
QVector<int> moveUpSelectedItems( QListView * view, AListModel< Item > & model )
{
	QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty())
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return {};
	}

	// the list of indexes is not sorted, they are in the order in which user selected them
	// but for the move, we need them sorted in ascending order
	QVector<int> selectedIndexesAsc;
	for (const QModelIndex & index : selectedIndexes)
		selectedIndexesAsc.push_back( index.row() );
	std::sort( selectedIndexesAsc.begin(), selectedIndexesAsc.end(), []( int idx1, int idx2 ) { return idx1 < idx2; } );

	// if the selected items are at the bottom, do nothing
	if (selectedIndexesAsc.first() == 0)
	{
		return {};
	}

	int currentIdx = getCurrentItemIndex( view );

	deselectAllAndUnsetCurrent( view );

	model.orderAboutToChange();

	// do the move and select the new positions
	for (int selectedIdx : selectedIndexesAsc)
	{
		model.move( selectedIdx, selectedIdx - 1 );
		selectItemByIndex( view, selectedIdx - 1 );
	}

	model.orderChanged();

	if (currentIdx >= 1)                                // if the current item was not the first one,
	{
		setCurrentItemByIndex( view, currentIdx - 1 );  // set the previous one as current,
	}
	else                                                // otherwise
	{
		setCurrentItemByIndex( view, 0 );               // set the first one as current
	}

	return selectedIndexesAsc;
}

/// Moves all selected items down and updates the selection to point to the new position.
/** Pops up a warning box if nothing is selected. */
template< typename Item >
QVector<int> moveDownSelectedItems( QListView * view, AListModel< Item > & model )
{
	QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty())
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return {};
	}

	// the list of indexes is not sorted, they are in the order in which user selected them
	// but for the move, we need them sorted in descending order
	QVector<int> selectedIndexesDesc;
	for (const QModelIndex & index : selectedIndexes)
		selectedIndexesDesc.push_back( index.row() );
	std::sort( selectedIndexesDesc.begin(), selectedIndexesDesc.end(), []( int idx1, int idx2 ) { return idx1 > idx2; } );

	// if the selected items are at the top, do nothing
	if (selectedIndexesDesc.first() == model.size() - 1)
	{
		return {};
	}

	int currentIdx = getCurrentItemIndex( view );

	deselectAllAndUnsetCurrent( view );

	model.orderAboutToChange();

	// do the move and select the new positions
	for (int selectedIdx : selectedIndexesDesc)
	{
		model.move( selectedIdx, selectedIdx + 1 );
		selectItemByIndex( view, selectedIdx + 1 );
	}

	model.orderChanged();

	if (currentIdx < model.size() - 1)                    // if the current item was not the last one,
	{
		setCurrentItemByIndex( view, currentIdx + 1 );    // set the next one as current,
	}
	else                                                  // otherwise
	{
		setCurrentItemByIndex( view, model.size() - 1 );  // set the last one as current
	}

	return selectedIndexesDesc;
}

bool editItemAtIndex( QListView * view, int index );




//======================================================================================================================
//  complete update helpers for list-view


/// Gets a persistent item ID of the current item that survives node shifting, adding or removal.
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
auto getCurrentItemID( QListView * view, const AListModel< Item > & model ) -> std::result_of_t<decltype(&Item::getID)(Item)>
{
	int selectedItemIdx = getCurrentItemIndex( view );
	if (selectedItemIdx >= 0)
		return model[ selectedItemIdx ].getID();
	else
		return {};
}

/// Gets a persistent item ID of a selected item that survives node shifting, adding or removal.
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
auto getSelectedItemID( QListView * view, const AListModel< Item > & model ) -> std::result_of_t<decltype(&Item::getID)(Item)>
{
	int selectedItemIdx = getSelectedItemIndex( view );
	if (selectedItemIdx >= 0)
		return model[ selectedItemIdx ].getID();
	else
		return {};
}

/// Attempts to set a previous current item defined by its persistant itemID.
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
bool setCurrentItemByID( QListView * view, const AListModel< Item > & model, const std::result_of_t<decltype(&Item::getID)(Item)> & itemID )
{
	if (!itemID.isEmpty())
	{
		int newItemIdx = findSuch( model, [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
		{
			setCurrentItemByIndex( view, newItemIdx );
			return true;
		}
	}
	return false;
}

/// Attempts to select a previously selected item defined by its persistant itemID.
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
bool selectItemByID( QListView * view, const AListModel< Item > & model, const std::result_of_t<decltype(&Item::getID)(Item)> & itemID )
{
	if (!itemID.isEmpty())
	{
		int newItemIdx = findSuch( model, [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
		{
			selectItemByIndex( view, newItemIdx );
			return true;
		}
	}
	return false;
}

/// Gets persistent item IDs that survive node shifting, adding or removal.
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
auto getSelectedItemIDs( QListView * view, const AListModel< Item > & model ) -> QVector< std::result_of_t<decltype(&Item::getID)(Item)> >
{
	QVector< decltype( Item{}.getID() ) > itemIDs;
	for (int selectedItemIdx : getSelectedItemIndexes( view ))
		itemIDs.append( model[ selectedItemIdx ].getID() );
	return itemIDs;
}

/// Attempts to select previously selected items defined by their persistant itemIDs.
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
void selectItemsByIDs( QListView * view, const AListModel< Item > & model, const QVector< std::result_of_t<decltype(&Item::getID)(Item)> > & itemIDs )
{
	for (const auto & itemID : itemIDs)
	{
		int newItemIdx = findSuch( model, [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
			selectItemByIndex( view, newItemIdx );
	}
}

/// Compares two selections of persistent item IDs.
template< typename ItemID >
bool areSelectionsEqual( const QVector< ItemID > & selection1, const QVector< ItemID > & selection2 )
{
	// The selected indexes are normally ordered in the order in which the user selected them.
	// So to be able to compare them we need to normalize the order first.

	QVector< ItemID > orderedSelection1;
	for (const ItemID & id : selection1)
		orderedSelection1.push_back( id );
	std::sort( orderedSelection1.begin(), orderedSelection1.end() );

	QVector< ItemID > orderedSelection2;
	for (const ItemID & id : selection2)
		orderedSelection2.push_back( id );
	std::sort( orderedSelection2.begin(), orderedSelection2.end() );

	return orderedSelection1 == orderedSelection2;
}

/// Fills a list with entries found in a directory.
template< typename Item >
void updateListFromDir( AListModel< Item > & model, QListView * view, const QString & dir, bool recursively,
                        const PathContext & pathContext, std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	// Doing a differential update (deleting only things that were deleted and adding only things that were added)
	// is not worth here. It's too complicated and prone to bugs and its advantages are too small.
	// Instead we just clear everything and then load it from scratch according to the current state of the directory
	// and update selection and scroll bar.

	// note down the current scroll bar position
	auto scrollPos = view->verticalScrollBar()->value();

	// note down the current item
	auto currentItemID = getCurrentItemID( view, model );

	// note down the selected items
	auto selectedItemIDs = getSelectedItemIDs( view, model );  // empty string when nothing is selected

	deselectAllAndUnsetCurrent( view );

	model.startCompleteUpdate();  // this resets the highlighted item pointed to by a mouse cursor,
	                              // but that's an acceptable drawback, instead of making differential update
	model.clear();

	traverseDirectory( dir, recursively, EntryType::FILE, pathContext, [&]( const QFileInfo & file )
	{
		if (isDesiredFile( file ))
		{
			model.append( Item( file ) );
		}
	});

	model.finishCompleteUpdate();

	// restore the selection so that the same file remains selected
	selectItemsByIDs( view, model, selectedItemIDs );

	// restore the current item so that the same file remains current
	setCurrentItemByID( view, model, currentItemID );

	// restore the scroll bar position, so that it doesn't move when an item is selected
	view->verticalScrollBar()->setValue( scrollPos );
}




//======================================================================================================================
//  complete update helpers for combo-box


/// Gets a persistent item ID that survives node shifting, adding or removal.
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
auto getCurrentItemID( QComboBox * view, const AListModel< Item > & model ) -> std::result_of_t<decltype(&Item::getID)(Item)>
{
	int selectedItemIdx = view->currentIndex();
	if (selectedItemIdx >= 0)
		return model[ selectedItemIdx ].getID();
	else
		return {};
}

/// Attempts to select a previously selected item defined by persistant itemID.
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
bool setCurrentItemByID( QComboBox * view, const AListModel< Item > & model, const std::result_of_t<decltype(&Item::getID)(Item)> & itemID )
{
	if (!itemID.isEmpty())
	{
		int newItemIdx = findSuch( model, [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
		{
			view->setCurrentIndex( newItemIdx );
			return true;
		}
	}
	return false;
}

/// Fills a combo-box with entries found in a directory.
template< typename Item >
void updateComboBoxFromDir( AListModel< Item > & model, QComboBox * view, const QString & dir, bool recursively,
                            bool includeEmptyItem, const PathContext & pathContext,
                            std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	// note down the currently selected item
	QString lastText = view->currentText();

	view->setCurrentIndex( -1 );

	model.startCompleteUpdate();

	model.clear();

	// in combo-box item cannot be deselected, so we provide an empty item to express "no selection"
	if (includeEmptyItem)
		model.append( QString() );

	traverseDirectory( dir, recursively, EntryType::FILE, pathContext, [&]( const QFileInfo & file )
	{
		if (isDesiredFile( file ))
		{
			model.append( Item( file ) );
		}
	});

	model.finishCompleteUpdate();

	// restore the originally selected item, the selection will be reset if the item does not exist in the new content
	// because findText returns -1 which is valid value for setCurrentIndex
	view->setCurrentIndex( view->findText( lastText ) );
}




//======================================================================================================================
//  miscellaneous


/// Expands all parent nodes from the selected node up to the root node, so that the selected node is immediately visible.
void expandParentsOfNode( QTreeView * view, const QModelIndex & index );

/// Changes text color of this widget.
void setTextColor( QWidget * widget, QColor color );

/// Restores all colors of this widget to default.
void restoreColors( QWidget * widget );


#endif // WIDGET_UTILS_INCLUDED
