/*
Caliente's BodySlide
by Caliente

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "StdAfx.h"
#include "Windowsx.h"
#include "PreviewWindow.h"

bool PreviewWindow::has_registered = false;

WNDPROC OldViewProc;

PreviewWindow::PreviewWindow(void) {
}

PreviewWindow::~PreviewWindow(void) {
	SendMessage(mHwnd, WM_DESTROY, 0, 0);
}

PreviewWindow::PreviewWindow(HWND owner, NifFile* nif, char PreviewType, char* shapeName) {
	string title;
	if (PreviewType == SMALL_PREVIEW) {
		isSmall = true;
		title = "Preview: Small Size";
	}
	else {
		isSmall = false;
		title = "Preview: Large Size";
	}
	hOwner = owner;

	Create(title);
	AddMeshFromNif(nif, shapeName);
	ShowWindow(mHwnd, SW_SHOW);
	UpdateWindow(mHwnd);
}

PreviewWindow::PreviewWindow(HWND owner, vector<vector3>* verts, vector<triangle>* tris, float scale) {
	hOwner = owner;
	string title = "Shape Preview";
	Create(title);
	gls.AddMeshExplicit(verts, tris, NULL, "", scale);
	ShowWindow(mHwnd, SW_SHOW);
	UpdateWindow(mHwnd);
}

void PreviewWindow::AddMeshDirect(mesh* m) {
	gls.AddMeshDirect(m);
	//gls.AddMeshExplicit(m->verts,m->tris,1.0f);
}

void PreviewWindow::AddMeshFromNif(NifFile *nif, char *shapeName) {
	vector<string> shapeList;
	nif->GetShapeList(shapeList);
	mesh* m;
	for (int i = 0; i < shapeList.size(); i++) {
		if (shapeName && (shapeList[i] == shapeName)) {
			gls.AddMeshFromNif(nif, shapeList[i]);
			m = gls.GetMesh(shapeName);
			m->BuildTriAdjacency();
			auto shader = nif->GetShaderForShape(shapeList[i]);
			if (shader && m->smoothSeamNormals != false && !shader->IsSkinShader())
				ToggleSmoothSeams(m);
		} else if (shapeName) {
			continue;
		} else {
			gls.AddMeshFromNif(nif, shapeList[i]);
			m = gls.GetMesh(shapeList[i]);
			m->BuildTriAdjacency();
			auto shader = nif->GetShaderForShape(shapeList[i]);
			if (shader && m->smoothSeamNormals != false && !shader->IsSkinShader())
				ToggleSmoothSeams(m);
		}
	}
}

void PreviewWindow::RefreshMeshFromNif(NifFile* nif, char* shapeName){
	vector<string> shapeList;
	nif->GetShapeList(shapeList);
	mesh* m;
	if (shapeName == NULL)
		gls.DeleteAllMeshes();
	for (int i = 0; i < shapeList.size(); i++) {
		if (shapeName && (shapeList[i] == shapeName)) {
			gls.ReloadMeshFromNif(nif, shapeList[i]);
			m = gls.GetMesh(shapeName);
			m->BuildTriAdjacency();
			auto shader = nif->GetShaderForShape(shapeList[i]);
			if (shader && m->smoothSeamNormals != false && !shader->IsSkinShader())
				ToggleSmoothSeams(m);
			if (shapeTexIds.find(shapeName) != shapeTexIds.end()) {
				m->MatRef = shapeTexIds[shapeName];
			} else {
				AddNifShapeTexture(nif, string(shapeName));
			}
			
		} else if (shapeName) {
			continue;
		} else {
			gls.ReloadMeshFromNif(nif, shapeList[i]);
			m = gls.GetMesh(shapeList[i]);
			m->BuildTriAdjacency();
			auto shader = nif->GetShaderForShape(shapeList[i]);
			if (shader && m->smoothSeamNormals != false && !shader->IsSkinShader())
				ToggleSmoothSeams(m);
			if (shapeTexIds.find(shapeList[i]) != shapeTexIds.end()) {
				m->MatRef = shapeTexIds[shapeList[i]];
			} else {
				AddNifShapeTexture(nif, shapeList[i]);
			}					
		}
	}
	InvalidateRect(mGLWindow,NULL,FALSE);
}

void PreviewWindow::AddNifShapeTexture(NifFile* fromNif, const string& shapeName) {
	int shader;
	string texFile;
	fromNif->GetTextureForShape((string)shapeName, texFile, 0);
	auto sb = fromNif->GetShaderForShape((string)shapeName);
	if (sb && sb->IsSkinShader())
		shader = 1;
	else
		shader = 0;

	SetShapeTexture((string)shapeName, baseDataPath + "textures\\" + texFile, shader);
}

void PreviewWindow::Create(const string& title) {
	if (!has_registered) 
		registerClass();

	RECT ownerRect;
	GetWindowRect(hOwner, &ownerRect);
	
	int sX = GetSystemMetrics(SM_CXDLGFRAME) * 2;
	int sY = GetSystemMetrics(SM_CYDLGFRAME) * 2;
	sY += GetSystemMetrics(SM_CYSMCAPTION);
	mHwnd = CreateWindowExA(0, "GL_PREVIEW_WINDOW", title.c_str(), WS_OVERLAPPED | WS_THICKFRAME | WS_MAXIMIZEBOX| WS_SYSMENU, ownerRect.left + 20, 0, 768 + sX, 768 + sY, NULL, NULL, GetModuleHandle(NULL), NULL);
	SetWindowLong(mHwnd, GWL_USERDATA, (LONG)this);

	//query multisample caps just once to avoid laggy window activation.
	if (!gls.MultiSampleQueried()) {
		HWND queryMSWndow = CreateWindowA("STATIC", "Multisampletester", WS_CHILD | SS_OWNERDRAW | SS_NOTIFY, 0, 0, 768, 768, mHwnd, 0, GetModuleHandle(NULL), NULL);
		gls.QueryMultisample(queryMSWndow);
		DestroyWindow(queryMSWndow);
	}
	
	mGLWindow = CreateWindowA("STATIC", "Model Preview", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | SS_NOTIFY, 0, 0, 768, 768, mHwnd, 0, GetModuleHandle(NULL), NULL);
	OldViewProc = (WNDPROC)SetWindowLong(mGLWindow, GWL_WNDPROC, (LONG)GLWindowProc);
	SetWindowLong(mGLWindow, GWL_USERDATA, (LONG)this);
	
	gls.Initialize(mGLWindow);
	gls.SetStartingView(vec3(0, -5.0f, -15.0f), 768, 768, 65.0);
}

void PreviewWindow::registerClass() {
	WNDCLASSEX wcex;
	HINSTANCE hinst = GetModuleHandle(NULL);

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW ;
	wcex.lpfnWndProc	= GLPreviewWindowWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hinst;
	wcex.hIcon			= LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE + 1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= _T("GL_PREVIEW_WINDOW");
	wcex.hIconSm		= LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));

	RegisterClassEx(&wcex);
	has_registered = true;
}

void PreviewWindow::RightDrag(int dX, int dY) {
	gls.TurnTableCamera(dX);
	gls.PitchCamera(dY);
	InvalidateRect(mGLWindow, NULL, FALSE);
}
void PreviewWindow::LeftDrag(int dX, int dY) {
	gls.PanCamera(dX,dY);
	InvalidateRect(mGLWindow,NULL,FALSE);
}

void PreviewWindow::TrackMouse(int X, int Y) {
	gls.UpdateCursor(X,Y);
	InvalidateRect(mGLWindow,NULL,FALSE);
}


void PreviewWindow::MouseWheel(int dW) {
	gls.DollyCamera(dW);
	InvalidateRect(mGLWindow,NULL,FALSE);
}

void PreviewWindow::Pick(int X, int Y) {
	vec3 camVec;
	vec3 dirVec;
	float len;
	//gls.UnprojectCamera(camVec);
	gls.GetPickRay(X,Y,dirVec,camVec);
	len = sqrt(camVec.x*camVec.x + camVec.y*camVec.y + camVec.z*camVec.z);

	gls.AddVisRay(camVec,dirVec, len);
	InvalidateRect(mGLWindow,NULL,FALSE);
}


void PreviewWindow::Close() {
	DestroyWindow(this->mHwnd);
}

LRESULT CALLBACK GLPreviewWindowWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PreviewWindow* p = (PreviewWindow*)GetWindowLong(hWnd,GWL_USERDATA);
	LPDRAWITEMSTRUCT pDIS ;
	switch (message)
	{
	case WM_ERASEBKGND:
		return TRUE;
		break;
	case WM_LBUTTONDOWN:
		return TRUE;
		break;
	case WM_SIZE:
		p->SetSize(LOWORD(lParam),HIWORD(lParam));
		break;
	case WM_DRAWITEM:
		pDIS = (LPDRAWITEMSTRUCT)lParam;
		p->Render();
		break;
	case WM_DESTROY:
		if(p->isSmall)
			PostMessage(p->hOwner,MSG_PREVIEWCLOSING,0,(LPARAM)p);
		else
			PostMessage(p->hOwner,MSG_BIGPREVIEWCLOSING,0,(LPARAM)p);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK GLWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static bool rbuttonDown = false;
	static bool lbuttonDown = false;
	static bool isLDragging = false;
	static int lastX;
	static int lastY;
	int x;
	int y;
	PreviewWindow* p = (PreviewWindow*)GetWindowLong(hWnd,GWL_USERDATA);
	
	switch(message) {
		case WM_RBUTTONDOWN:
			rbuttonDown = true;
			lastX = GET_X_LPARAM(lParam);
			lastY = GET_Y_LPARAM(lParam);
			SetCapture(hWnd);
			
			break;
		case WM_RBUTTONUP:
			rbuttonDown = false;
			lastX = GET_X_LPARAM(lParam);
			lastY = GET_Y_LPARAM(lParam);
			ReleaseCapture();
			break;
		case WM_LBUTTONDOWN:
			lbuttonDown = true;
			SetCapture(hWnd);
			break;
		case WM_LBUTTONUP:
			lbuttonDown = false;
		//	if(!isLDragging) {
		//		p->Pick(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		//	}
			isLDragging = false;
			ReleaseCapture();
			break;
		case WM_MOUSEMOVE:
			if(p->HasFocus()) {
				SetFocus(hWnd);
			}
			x = GET_X_LPARAM(lParam);
			y = GET_Y_LPARAM(lParam);
			if(rbuttonDown) {
				p->RightDrag(x-lastX,y-lastY);
			}
			if(lbuttonDown) {
				isLDragging = true;
				p->LeftDrag(x-lastX,y-lastY);
			}
			if(!rbuttonDown && !lbuttonDown) {
				p->TrackMouse(x,y);
			}
			lastX = x;
			lastY = y;
			break;
		case WM_KEYUP:
			switch(wParam) {
			case 0x4e:
				p->ToggleSmoothSeams();
				break;
			case 0x54:
				p->ToggleTextures();
				break;
			case 0x57:
				p->ToggleWireframe();
				break;
			case 0x4c:
				p->ToggleLighting();
				break;
			case 0x51:
				p->ToggleEditMode();
				break;
			} 
			break;
		case WM_MOUSEWHEEL: 
			p->MouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
			break;							
		default:
			return CallWindowProc(OldViewProc,hWnd,message,wParam, lParam);

	}
	return 0;
}