#include "SliderManager.h"
#include <sstream>
#include <algorithm>


WNDPROC PrevEditProc=NULL;

SliderManager::SliderManager(void)
{
	mSliderCount=0;
	bNeedReload = true;
}
/*
SliderManager::SliderManager(HWND ownerhWnd, HINSTANCE ownerhInst, HFONT appfont, int basectlID){
	SetOwnerInfo(ownerhWnd,ownerhInst,appfont,basectlID);
	mSliderCount = 0;
	bNeedReload = true;
}
*/
SliderManager::~SliderManager(void)
{
}/*
void SliderManager::ScrollSliders(int amount) {
	RECT r;
	RECT cr;
	RECT or;
	GetClientRect(owner,&or);
	int clipBottom = or.bottom-90;
//	int maxScroll = or.bottom - 160;
	int curscroll = GetScrollPos(this->owner,SB_VERT);
//	if(curscroll - amount < -30) return;
//	if(curscroll - amount > (mSliderCount*30- maxScroll)) return;

	for(int i =0;i<SlidersBig.size();i++) {
		GetWindowRect(SlidersBig[i].hWnd,&r);
		
		ScreenToClient(owner,(LPPOINT)&r);	
		GetClientRect(SlidersBig[i].hWnd,&cr);	
		MoveWindow(SlidersBig[i].hWnd,r.left,r.top + amount,cr.right,cr.bottom,TRUE);
		
		GetWindowRect(SlidersBig[i].labelWnd,&r);
		ScreenToClient(owner,(LPPOINT)&r);	
		SetWindowPos(SlidersBig[i].labelWnd,NULL,r.left,r.top+amount,0,0,SWP_NOSIZE | SWP_NOZORDER);

		GetWindowRect(SlidersBig[i].readoutWnd,&r);
		ScreenToClient(owner,(LPPOINT)&r);	
		SetWindowPos(SlidersBig[i].readoutWnd,NULL,r.left,r.top+amount,0,0,SWP_NOSIZE | SWP_NOZORDER);
		if(r.top+amount > clipBottom || (r.top+amount+cr.bottom) < 95) {
			ShowWindow(SlidersBig[i].hWnd,SW_HIDE);
			ShowWindow(SlidersBig[i].labelWnd,SW_HIDE);
			ShowWindow(SlidersBig[i].readoutWnd,SW_HIDE);
		} else {
			ShowWindow(SlidersBig[i].hWnd,SW_SHOW);
			ShowWindow(SlidersBig[i].labelWnd,SW_SHOW);
			ShowWindow(SlidersBig[i].readoutWnd,SW_SHOW);
		}
	}
	for(int i =0;i<SlidersSmall.size();i++) {
		GetWindowRect(SlidersSmall[i].hWnd,&r);
		GetClientRect(SlidersSmall[i].hWnd,&cr);
		ScreenToClient(owner,(LPPOINT)&r);		
		MoveWindow(SlidersSmall[i].hWnd,r.left,r.top + amount,cr.right,cr.bottom,TRUE);

		GetWindowRect(SlidersSmall[i].labelWnd,&r);
		ScreenToClient(owner,(LPPOINT)&r);	
		SetWindowPos(SlidersSmall[i].labelWnd,NULL,r.left,r.top+amount,0,0,SWP_NOSIZE | SWP_NOZORDER);

		GetWindowRect(SlidersSmall[i].readoutWnd,&r);
		ScreenToClient(owner,(LPPOINT)&r);	
		SetWindowPos(SlidersSmall[i].readoutWnd,NULL,r.left,r.top+amount,0,0,SWP_NOSIZE | SWP_NOZORDER);
		if(r.top+amount > clipBottom || (r.top+amount+cr.bottom) < 95) {
			ShowWindow(SlidersSmall[i].hWnd,SW_HIDE);
			ShowWindow(SlidersSmall[i].labelWnd,SW_HIDE);
			ShowWindow(SlidersSmall[i].readoutWnd,SW_HIDE);
		} else {
			ShowWindow(SlidersSmall[i].hWnd,SW_SHOW);
			ShowWindow(SlidersSmall[i].labelWnd,SW_SHOW);
			ShowWindow(SlidersSmall[i].readoutWnd,SW_SHOW);
		}
	}
	//GetScrollPos(this->owner,SB_VERT);
	SetScrollPos(this->owner,SB_VERT, curscroll - amount,TRUE);

}
*/

void SliderManager::AddSlidersInSet(SliderSet& inSet, bool hideAll) {
	int sz = inSet.size();
	map<string, float>::iterator reqIter;

	for (int i = 0; i < sz; i++) {
		if (inSet[i].bHidden || hideAll) {
			AddHiddenSlider(inSet[i].Name, inSet[i].bInvert, inSet[i].bZap, inSet[i].bUV);
		} else {
			if (inSet[i].bZap) {
				AddZapSlider(inSet[i].Name);
			} else if (inSet[i].bUV) {
				AddUVSlider(inSet[i].Name, inSet[i].bInvert);
			} else {
				AddSlider(inSet[i].Name, inSet[i].bInvert);
			}
		}
		SetSliderDefaults(inSet[i].Name, inSet[i].defBigValue / 100.0f, inSet[i].defSmallValue / 100.0f);
		if (inSet[i].bClamp) {
			SetClampSlider(inSet[i].Name);
		}

		for (int j = 0;j<inSet[i].dataFiles.size(); j++) {
			AddSliderLink(inSet[i].Name, inSet[i].dataFiles[j].dataName);
		}

		for (reqIter = inSet[i].requirements.begin(); reqIter != inSet[i].requirements.end(); ++reqIter) {
			AddSliderTrigger(reqIter->first, inSet[i].Name, reqIter->second, 0);
		}
	}
}

void SliderManager::AddSlider(const string& name, bool invert, const string& dataSetName) {
	Slider s;
	s.Name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);
	s.Value = 0;
	s.defValue = 0;
	s.invert = invert;
	s.zap = false;
	s.clamp = false;
	s.uv = false;
	s.changed = false;
	//WNDPROC tempProc;
	//int ypos = 100 + 30 * mSliderCount;

	SlidersSmall.push_back(s);
	SlidersBig.push_back(s);
	mSliderCount++;
}

void SliderManager::AddUVSlider(const string& name, bool invert, const string& dataSetName) {
	Slider s;
	s.Name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);
	s.Value = 0;
	s.defValue = 0;
	s.invert = invert;
	s.zap = false;
	s.clamp = false;
	s.uv = true;
	s.changed = false;

	//int ypos = 100 + 30 * mSliderCount;

	SlidersSmall.push_back(s);
	SlidersBig.push_back(s);
	mSliderCount++;
}

void SliderManager::AddZapSlider(const string& name, const string& dataSetName) {
	Slider s;
	s.Name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);
	s.Value = 0;
	s.defValue = 0;
	s.invert = false;
	s.zap = true;
	s.clamp = false;
	s.uv = false;
	s.changed = false;

	//int ypos = 100 + 30 * mSliderCount;
	SlidersSmall.push_back(s);
	SlidersBig.push_back(s);
	mSliderCount++;
}

//void SliderManager::AddZapData(const string& name,const string& dataSetName) {
//	ZapDataLinks[name] = dataSetName;
//}

void SliderManager::AddHiddenSlider(const string& name, bool invert, bool isZap, bool isUv, const string& dataSetName) {
	Slider s;
	s.Name = name;
	if (dataSetName.length() > 0)
		s.linkedDataSets.push_back(dataSetName);
	s.Value = 0;
	s.defValue = 0;
	s.invert = invert;
	s.zap = isZap;
	s.clamp = false;
	s.uv = isUv;
	s.changed = false;
	//s.hWnd = 0;
	SlidersSmall.push_back(s);
	SlidersBig.push_back(s);
}

void SliderManager::SetSliderDefaults(const string& slider, float bigVal, float smallVal) {
	int i;
	for (i = 0; i < SlidersBig.size(); i++) {
		if (SlidersBig[i].Name == slider) {
			SlidersBig[i].defValue = bigVal;
		}
	}
	for (i = 0; i < SlidersSmall.size(); i++) {
		if (SlidersSmall[i].Name == slider) {
			SlidersSmall[i].defValue = smallVal;
		}
	}
}

void SliderManager::SetClampSlider(const string& slider) {
	int i;
	for (i = 0; i < SlidersBig.size(); i++) {
		if (SlidersBig[i].Name == slider) {
			SlidersBig[i].clamp = true;
		}
	}
	for (i = 0; i < SlidersSmall.size(); i++) {
		if (SlidersSmall[i].Name == slider) {
			SlidersSmall[i].clamp = true;
		}
	}
}
void SliderManager::AddSliderLink(const string& slider, const string& dataSetName) {
	int i;
	for (i = 0; i < SlidersBig.size(); i++) {
		if (SlidersBig[i].Name == slider) {
			SlidersBig[i].linkedDataSets.push_back(dataSetName);
		}
	}
	for (i = 0; i < SlidersSmall.size(); i++) {
		if (SlidersSmall[i].Name == slider) {
			SlidersSmall[i].linkedDataSets.push_back(dataSetName);
		}
	}
}

void SliderManager::AddSliderTrigger(const string& slider, const string& targetSlider, float triggerVal, unsigned int triggerType) {
	int i;
	int targetIdx = -1;
	int sliderIdx = -1;
	SliderNotifyTrigger trigger;
	trigger.triggerValue = triggerVal;
	trigger.triggerType = triggerType;
	for(i =0;i<SlidersBig.size();i++) {
		if(SlidersBig[i].Name==slider) {
			sliderIdx = i;
		}
		if(SlidersBig[i].Name==targetSlider) {
			targetIdx = i;
		}
	}
	if(sliderIdx >=0 && targetIdx >=0 ){
		trigger.targetSlider = SlidersBig[targetIdx].Name;
		SlidersBig[sliderIdx].triggers.push_back(trigger);
	}
	sliderIdx = -1; targetIdx = -1;
	for(i =0;i<SlidersSmall.size();i++) {
		if(SlidersSmall[i].Name==slider) {
			sliderIdx = i;
		}
		if(SlidersSmall[i].Name==targetSlider) {
			targetIdx = i;
		}
	}
	if(sliderIdx >=0 && targetIdx >=0 ){
		trigger.targetSlider = SlidersSmall[targetIdx].Name;
		SlidersSmall[sliderIdx].triggers.push_back(trigger);
	}

}
/*
bool SliderManager::SetSliderFromEditControlID(int controlID) {
	char str[256];
	float val;
	if(controlID < 200) return false;
	if(controlID > 400) return false;
	if(controlID >= 200 && controlID<300) {	// small slider
		GetWindowText(SlidersSmall[controlID-200].readoutWnd,str,256);
		val = atof(str);
		SlidersSmall[controlID-200].Set(val/100,true);
		_snprintf(str,256,"%d%%",val);
		return true;
	} else {
		GetWindowText(SlidersBig[controlID-300].readoutWnd,str,256);
		val = atof(str);
		SlidersBig[controlID-300].Set(val/100,true);
		_snprintf(str,256,"%d%%",val);
		return true;
	}
	return false;
}*/
/*
void SliderManager::SetSlider(HWND sliderID, float val) {
	int i;
	for(i =0;i<SlidersBig.size();i++) {
		if(SlidersBig[i].hWnd==sliderID) {
			if(SlidersBig[i].zap) {
				FlagReload(true);
				if(val > 0) val = 1.0;
				SlidersSmall[i].Set(val,true);
			}
			SlidersBig[i].Set(val);

			return;
		}
	}
	for(i =0;i<SlidersSmall.size();i++) {
		if(SlidersSmall[i].hWnd==sliderID) {
			if(SlidersSmall[i].zap) {
				FlagReload(true);
				if(val > 0) val = 1.0;
				SlidersBig[i].Set(val,true);
			}
			SlidersSmall[i].Set(val);

			return;
		}
	}

}
*/

float SliderManager::GetSlider(const string& slider, bool isSmall) {
	int i;
	if (!isSmall) {
		for (i = 0; i < SlidersBig.size(); i++) {
			if (SlidersBig[i].Name == slider)
				return SlidersBig[i].Get();
		}
	} else {
		for (i = 0; i < SlidersSmall.size(); i++) {
			if (SlidersSmall[i].Name == slider)
				return SlidersSmall[i].Get();
		}
	}
	return 0.0f;
}

void SliderManager::SetSlider(const string& slider, bool isSmall, float val) {
	int i;
	if(!isSmall) {
		for(i =0;i<SlidersBig.size();i++) {
			if(SlidersBig[i].Name==slider) {
				if(SlidersBig[i].zap) {
					FlagReload(true);
					if(val > 0) val = 1.0;
				}
				SlidersBig[i].Set(val);
				return;
			}
		}
	} else {
		for(i =0;i<SlidersSmall.size();i++) {
			if(SlidersSmall[i].Name==slider) {
				if(SlidersSmall[i].zap) {
					FlagReload(true);
					if(val > 0) val = 1.0;
				}
				SlidersSmall[i].Set(val);
				return;
			}
		}
	}
}

void SliderManager::SetChanged(const string& slider, bool isSmall) {
	int i;
	if(!isSmall) {
		for(i =0;i<SlidersBig.size();i++) {
			if(SlidersBig[i].Name==slider) {
				SlidersBig[i].changed = true;
				return;
			}
		}
	} else {
		for(i =0;i<SlidersSmall.size();i++) {
			if(SlidersSmall[i].Name==slider) {
				SlidersSmall[i].changed = true;
				return;
			}
		}
	}
}

// for speed, assumes the slider is small if the HWND isn't found in the large sliders set ... this of course could be drastically wrong.
/*bool SliderManager::SliderIsSmall(HWND sliderID) {
	int i;
	for(i =0;i<SlidersBig.size();i++) {
		if(SlidersBig[i].hWnd==sliderID) {
			return false;
		}
	}
	return true;	
}

*/

float SliderManager::GetBigPresetValue(const string& presetName, const string& sliderName, float defVal ) {
	float ps;
	if(!presetCollection.GetBigPreset(presetName,sliderName,ps)) {
		ps =defVal;
	} 
	return ps;
}

float SliderManager::GetSmallPresetValue(const string& presetName, const string& sliderName, float defVal ){
	float ps;
	if(!presetCollection.GetSmallPreset(presetName,sliderName,ps)) {
		ps =defVal;
	} 
	return ps;
}

void SliderManager::InitializeSliders(const string& presetName) {
	int i;
	float ps;
	for(i =0;i<SlidersBig.size();i++) {
		if(!presetCollection.GetBigPreset(presetName,SlidersBig[i].Name,ps)) {
			ps = SlidersBig[i].defValue;
		} 
		SlidersBig[i].Set(ps);		
//		SendMessage(SlidersBig[i].hWnd,TBM_SETPOS,TRUE,ps*100);
	}
	for(i =0;i<SlidersSmall.size();i++) {
		if(!presetCollection.GetSmallPreset(presetName,SlidersSmall[i].Name,ps)) {
			ps = SlidersSmall[i].defValue;
		} 
		SlidersSmall[i].Set(ps);
//		SendMessage(SlidersSmall[i].hWnd,TBM_SETPOS,TRUE,ps*100);
	}
}

bool SliderManager::SliderHasChanged(const string& slider, bool getBig) {
	int i;
	if(getBig) {
		for(i = 0; i < SlidersBig.size(); i++) {
			if(SlidersBig[i].Name == slider) {
				return(SlidersBig[i].defValue != SlidersBig[i].Value || SlidersBig[i].changed);
			}
		}
	} else {
		for(i = 0; i < SlidersSmall.size(); i++) {
			if(SlidersSmall[i].Name == slider) {
				return(SlidersSmall[i].defValue != SlidersSmall[i].Value || SlidersBig[i].changed);
			}
		}
	}
	return false;
}

float SliderManager::SliderValue(const string& slider, bool getBig) {
	int i;
	if(getBig) {
		for(i =0;i<SlidersBig.size();i++) {
			if(SlidersBig[i].Name == slider) {
				return(SlidersBig[i].Value);
			}
		}
	} else {
		for(i =0;i<SlidersSmall.size();i++) {
			if(SlidersSmall[i].Name == slider) {
				return(SlidersSmall[i].Value);
			}
		}
	}
	return 0.0f;
}

void SliderManager::GetSmallSliderList(vector<string>& names) {
	int i;
	for(i =0;i<SlidersSmall.size();i++) {
		names.push_back(SlidersSmall[i].Name);
	}
}
void SliderManager::GetBigSliderList(vector<string>& names) {
	int i;
	for(i =0;i<SlidersBig.size();i++) {
		names.push_back(SlidersBig[i].Name);
	}
}

/*

LRESULT CALLBACK SliderEditControlTabHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	SliderManager* sm = (SliderManager*)GetWindowLong(hWnd,GWL_USERDATA);
	switch(msg) {
		case WM_SETFOCUS:
			SendMessage(hWnd,EM_SETSEL,0,-1);
		case WM_CHAR:
			if(wParam == 0x09) {				
				if(GetKeyState(VK_SHIFT) & 0x80)
					sm->NextControl(hWnd, true);
				else 
					sm->NextControl(hWnd, false);
				return TRUE;
			}if(wParam == 0x0D) {
				sm->NextControl(hWnd, false);
				return TRUE;
			}if(wParam == 0x0A) {
				sm->NextControl(hWnd, true);
				return TRUE;
			}
		default:
			return CallWindowProc(PrevEditProc,hWnd,msg,wParam, lParam);
	}
}
*/