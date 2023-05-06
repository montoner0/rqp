#pragma once
#include "guids.h"

#define _tmemset wmemset

#define _OpenPlugin           OpenW
#define EXITFARPARMS const struct ExitInfo *ei
#define CONFIGUREPARMS const struct ConfigureInfo *ci
#define DIALOGINIT(pluginid,dlguid,items,openerror) Info.DialogInit(pluginid, &dlguid, -1, -1,  DLG_WIDTH, DLG_HEIGHT, L"Contents", items, _countof(items), 0, FDLG_SMALLDIALOG, dlgProc, (FPAR2)openerror)

#define _ClosePlugin          ClosePluginW
#define _Compare              CompareW
#define _Configure            ConfigureW
#define _DeleteFiles          DeleteFilesW
#define _ExitFAR              ExitFARW
#define _FreeFindData         FreeFindDataW
#define _FreeVirtualFindData  FreeVirtualFindDataW
#define _GetFiles             GetFilesW
#define _GetFindData          GetFindDataW
#define _GetMinFarVersion     GetMinFarVersionW
#define _GetOpenPluginInfo    GetOpenPluginInfoW
#define _GetPluginInfo        GetPluginInfoW
#define _GetVirtualFindData   GetVirtualFindDataW
#define _MakeDirectory        MakeDirectoryW
#define _OpenFilePlugin       OpenFilePluginW
#define _ProcessDialogEvent   ProcessDialogEventW
#define _ProcessEditorEvent   ProcessEditorEventW
#define _ProcessEditorInput   ProcessEditorInputW
#define _ProcessEvent         ProcessEventW
#define _ProcessHostFile      ProcessHostFileW
#define _ProcessKey           ProcessKeyW
#define _ProcessViewerEvent   ProcessViewerEventW
#define _PutFiles             PutFilesW
#define _SetDirectory         SetDirectoryW
#define _SetFindList          SetFindListW
#define _SetStartupInfo       SetStartupInfoW

#define _pdata PtrData

#define _PControl PanelControl
#define PLUGIN_ID &g_plguid
#define PLUGIN_ID_TYPE GUID*
typedef void* FPAR2;
typedef intptr_t FINT;
typedef intptr_t DLGPROCRETVAL;
typedef FarMenuItem FARMENUITEM;
typedef size_t FSIZET;
#define FCTL_GETPANELDIR FCTL_GETPANELDIRECTORY
#define FMENU_USEEXT 0
#define DN_GETDIALOGINFO DM_GETDIALOGINFO
#define INPUTBOXID(...) Info.InputBox(PLUGIN_ID, &g_miscguid, __VA_ARGS__)
#define PINPUTBOXID(...) Info->InputBox(PLUGIN_ID, &g_miscguid, __VA_ARGS__)
#define F3STRUCTSIZE(x) (x).StructSize=sizeof(x)
#define INITPANELITEMSTRUCT(ppi,size,resstruct) FarGetPluginPanelItem fgppi={sizeof(fgppi),size,ppi}; FPAR2 resstruct=&fgppi

//Type,        X1,Y1,X2,Y2,Param,History,Mask,Flags,Data,MaxLength,UserData,Reserved[2]
#define SETFARDIALOGITEM(type,x1,y1,x2,y2,flags,data) {type, x1,y1,x2,y2, 0, 0, 0, flags, data}
#define SETFARDIALOGITEM2(type,x1,y1,flags,data) {type, x1,y1,x1,y1, 0, 0, 0, flags, data}
