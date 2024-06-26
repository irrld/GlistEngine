/*
 * gGUIDropdownList.h
 *
 *  Created on: 19 A�u 2022
 *      Author: sevval
 */


#ifndef UI_GGUIDROPDOWNLIST_H_
#define UI_GGUIDROPDOWNLIST_H_

#include "gGUIContainer.h"
#include "gGUISizer.h"
#include "gGUIButton.h"
#include "gGUIImageButton.h"
#include "gGUITextbox.h"
#include "gGUITreelist.h"
#include "gGUIFrame.h"
#include "gBaseCanvas.h"
#include "gBaseApp.h"

class gGUIDropdownList: public gGUIContainer {
public:
	gGUITreelist list;
	virtual void onGUIEvent(int guiObjectId, int eventType, int sourceEventType, std::string value1 = "", std::string value2 = "") override;
	gGUIDropdownList();
	virtual ~gGUIDropdownList();
	void set(gBaseApp* root, gBaseGUIObject* topParentGUIObject, gBaseGUIObject* parentGUIObject, int parentSlotLineNo, int parentSlotColumnNo, int x, int y, int w, int h) override;
	void draw() override;
	void setParentFrame(gGUIForm* form);
	void setParentForm(gGUIForm* form);
	void mousePressed(int x, int y, int button) override;
	void mouseReleased(int x, int y, int button) override;
	void mouseScrolled(int x, int y) override;
	void setfirstTitle();
	void setSelectedTitle();
	void addElement(gGUITreelist::Element* element);
	void addElement(gGUITreelist::Element* element, gGUITreelist::Element* parentelement);
	void clearTitle();
	void clear();
	void setDisabled(bool isDisabled);

	std::string& getSelectedTitle();

	int calculateContentHeight() override;

private:
	friend class gGUIToolbar;

	gGUISizer listsizer;
	gGUIButton button;
	gGUIImageButton ibutton;
	gGUITextbox textbox;
	gGUITreelist::Element* rootelement;
	int textboxw, buttonw;
	int textboxh;
	int listx, listy, listw;
	bool listopened, selectedline, listexpanded, pressedonlist, buttonpressed, ispressed;
	bool lopened;
	gGUIForm* frame;
	std::string fTitle;
	bool isdisabled;
};

#endif /* UI_GGUIDROPDOWNLIST_H_ */
