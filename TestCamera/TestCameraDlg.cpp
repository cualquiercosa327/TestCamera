
// TestCameraDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "TestCamera.h"
#include "TestCameraDlg.h"
#include "afxdialogex.h"
#include <ks.h>
#include <ksmedia.h>
#include <windowsx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestCameraDlg �Ի���



CTestCameraDlg::CTestCameraDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_TESTCAMERA_DIALOG, pParent)
	, g_hdevnotify(0)
	, g_pCapture(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestCameraDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MEDIA_TYPE, m_oMediaType);
	DDX_Control(pDX, IDC_HWENC, m_oHwEnc);
	DDX_Control(pDX, IDC_SLIDER_BRIGHT, m_sliderBright);
	DDX_Control(pDX, IDC_AUTO_BRIGHT, m_btAutoBright);
}

BEGIN_MESSAGE_MAP(CTestCameraDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CTestCameraDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CTestCameraDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_CAPTURE, &CTestCameraDlg::OnBnClickedCapture)
	ON_CBN_SELCHANGE(IDC_DEVICE_LIST, &CTestCameraDlg::OnCbnSelchangeDeviceList)
	ON_BN_CLICKED(IDC_CAPTURE_MP4, &CTestCameraDlg::OnBnClickedCaptureMp4)
	ON_BN_CLICKED(IDC_CAPTURE_WMV, &CTestCameraDlg::OnBnClickedCaptureWmv)
	ON_CBN_SELCHANGE(IDC_MEDIA_TYPE, &CTestCameraDlg::OnCbnSelchangeMediaType)
	ON_NOTIFY(TRBN_THUMBPOSCHANGING, IDC_SLIDER_BRIGHT, &CTestCameraDlg::OnTRBNThumbPosChangingSliderBright)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CTestCameraDlg ��Ϣ�������

BOOL CTestCameraDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	HWND    hEdit = ::GetDlgItem(m_hWnd, IDC_OUTPUT_FILE);
	::SetWindowTextW(hEdit, L"Y:\\test.mp4");
	m_oHwEnc.SetCheck(BST_CHECKED);

	// Initialize the COM library
	HRESULT hr = S_OK;
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	// Initialize Media Foundation
	if (SUCCEEDED(hr))
	{
		hr = MFStartup(MF_VERSION);
	}

	// Register for device notifications
	if (SUCCEEDED(hr))
	{
		DEV_BROADCAST_DEVICEINTERFACE di = { 0 };

		di.dbcc_size = sizeof(di);
		di.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		di.dbcc_classguid = KSCATEGORY_CAPTURE;

		g_hdevnotify = RegisterDeviceNotification(
			m_hWnd,
			&di,
			DEVICE_NOTIFY_WINDOW_HANDLE
		);

		if (g_hdevnotify == NULL)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	// Enumerate the video capture devices.
	if (SUCCEEDED(hr))
	{
		hr = UpdateDeviceList();
	}

	if (SUCCEEDED(hr))
	{
		UpdateUI();

		if (g_devices.Count() == 0)
		{
			::MessageBox(
				m_hWnd,
				TEXT("Could not find any video capture devices."),
				TEXT("MFCaptureToFile"),
				MB_OK
			);
		}

		QueryMediaTypeList();

		StartCamera();
		UpdateVideoProcParams();
	}
	else
	{
		OnCloseDialog();
		::EndDialog(m_hWnd, 0);
	}

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CTestCameraDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CTestCameraDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

HRESULT CTestCameraDlg::UpdateDeviceList()
{
	HRESULT hr = S_OK;

	WCHAR *szFriendlyName = NULL;

	HWND hCombobox = ::GetDlgItem(m_hWnd, IDC_DEVICE_LIST);

	ComboBox_ResetContent(hCombobox);

	g_devices.Clear();

	hr = g_devices.EnumerateDevices();

	if (FAILED(hr)) { goto done; }

	for (UINT32 iDevice = 0; iDevice < g_devices.Count(); iDevice++)
	{
		// Get the friendly name of the device.

		hr = g_devices.GetDeviceName(iDevice, &szFriendlyName);

		if (FAILED(hr)) { goto done; }

		// Add the string to the combo-box. This message returns the index in the list.

		int iListIndex = ComboBox_AddString(hCombobox, szFriendlyName);
		if (iListIndex == CB_ERR || iListIndex == CB_ERRSPACE)
		{
			hr = E_FAIL;
			goto done;
		}

		// The list might be sorted, so the list index is not always the same as the
		// array index. Therefore, set the array index as item data.

		int result = ComboBox_SetItemData(hCombobox, iListIndex, iDevice);

		if (result == CB_ERR)
		{
			hr = E_FAIL;
			goto done;
		}

		CoTaskMemFree(szFriendlyName);
		szFriendlyName = NULL;
	}

	if (g_devices.Count() > 0)
	{
		// Select the first item.
		ComboBox_SetCurSel(hCombobox, 0);
	}

done:
	return hr;
}

void CTestCameraDlg::UpdateUI()
{
	BOOL bEnable = (g_devices.Count() > 0);     // Are there any capture devices?
	BOOL bPreviewing = (g_pCapture != NULL);     // Is video capture in progress now?
	BOOL bCapturing = FALSE;
	if (g_pCapture && g_pCapture->IsRecording())
	{
		bCapturing = TRUE;
	}

	HWND hButton = ::GetDlgItem(m_hWnd, IDC_CAPTURE);

	if (bCapturing)
	{
		::SetWindowText(hButton, L"Stop Capture");
	}
	else
	{
		::SetWindowText(hButton, L"Start Capture");
	}

	EnableDialogControl(m_hWnd, IDC_CAPTURE, bCapturing || bEnable);

	EnableDialogControl(m_hWnd, IDC_DEVICE_LIST, !bCapturing && bEnable);

	// The following cannot be changed while capture is in progress,
	// but are OK to change when there are no capture devices.

 	EnableDialogControl(m_hWnd, IDC_CAPTURE_MP4, !bCapturing);
 	EnableDialogControl(m_hWnd, IDC_CAPTURE_WMV, !bCapturing);
 	EnableDialogControl(m_hWnd, IDC_OUTPUT_FILE, !bCapturing);
}

void CTestCameraDlg::EnableDialogControl(HWND hDlg, int nIDDlgItem, BOOL bEnable)
{
	HWND hwnd = ::GetDlgItem(hDlg, nIDDlgItem);

	if (!bEnable &&  hwnd == ::GetFocus())
	{
		// When disabling a control that has focus, set the 
		// focus to the next control.

		::SendMessage(::GetParent(hwnd), WM_NEXTDLGCTL, 0, FALSE);
	}
	::EnableWindow(hwnd, bEnable);
}

void CTestCameraDlg::OnCloseDialog()
{
	if (g_pCapture)
	{
		g_pCapture->EndCaptureSession();
	}

	SafeRelease(&g_pCapture);

	g_devices.Clear();

	if (g_hdevnotify)
	{
		UnregisterDeviceNotification(g_hdevnotify);
	}

	MFShutdown();
	CoUninitialize();
}

void CTestCameraDlg::UpdateParam(EncodingParameters & params)
{
	if (BST_CHECKED == IsDlgButtonChecked(IDC_CAPTURE_WMV))
	{
		params.subtype = MFVideoFormat_WMV3;
	}
	else
	{
		params.subtype = MFVideoFormat_H264;
	}

	if (BST_CHECKED == IsDlgButtonChecked(IDC_HWENC))
	{
		params.hwenc = 1;
	}
	else
	{
		params.hwenc = 0;
	}

	params.bitrate = TARGET_BIT_RATE;
	int nIndex = m_oMediaType.GetCurSel();
	if (nIndex >= 0)
	{
		params.mediatype = m_oMediaType.GetItemData(nIndex);
	}
	else
	{
		params.mediatype = 0;
	}
}



void CTestCameraDlg::StopCamra()
{
	HRESULT hr = S_OK;

	hr = g_pCapture->EndCaptureSession();

	SafeRelease(&g_pCapture);

	UpdateDeviceList();

	// NOTE: Updating the device list releases the existing IMFActivate 
	// pointers. This ensures that the current instance of the video capture 
	// source is released.

	UpdateUI();

	if (FAILED(hr))
	{
		NotifyError(L"Error stopping capture. File might be corrupt.", hr);
	}
}

void CTestCameraDlg::StopCapture()
{
	HRESULT hr = S_OK;

	hr = g_pCapture->EndRecord();
	
	// NOTE: Updating the device list releases the existing IMFActivate 
	// pointers. This ensures that the current instance of the video capture 
	// source is released.

	UpdateUI();

	if (FAILED(hr))
	{
		NotifyError(L"Error stopping capture. File might be corrupt.", hr);
	}
}
void CTestCameraDlg::StartCapture()
{
	EncodingParameters params;
	UpdateParam(params);

	HRESULT hr = S_OK;
	WCHAR   pszFile[MAX_PATH] = { 0 };
	HWND    hEdit = ::GetDlgItem(m_hWnd, IDC_OUTPUT_FILE);

	IMFActivate *pActivate = NULL;

	// Get the name of the target file.

	if (0 == ::GetWindowText(hEdit, pszFile, MAX_PATH))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	// Create the media source for the capture device.

	if (SUCCEEDED(hr))
	{
		hr = GetSelectedDevice(&pActivate);
	}

	// Start capturing.

	if (SUCCEEDED(hr))
	{
		hr = CCapture::CreateInstance(m_hWnd, &g_pCapture);
	}

	if (SUCCEEDED(hr))
	{
		hr = g_pCapture->StartRecord(pActivate, pszFile, params);
	}

	if (SUCCEEDED(hr))
	{
		UpdateUI();
	}


	SafeRelease(&pActivate);

	if (FAILED(hr))
	{
		NotifyError(L"Error starting capture.", hr);
	}
}

void CTestCameraDlg::StartCamera()
{
	EncodingParameters params;
	UpdateParam(params);
	IMFActivate *pActivate = NULL;
	HRESULT hr = S_OK;

	// Create the media source for the capture device.

	if (SUCCEEDED(hr))
	{
		hr = GetSelectedDevice(&pActivate);
	}

	// Start capturing.

	if (SUCCEEDED(hr))
	{
		hr = CCapture::CreateInstance(m_hWnd, &g_pCapture);
	}

	if (SUCCEEDED(hr))
	{
		g_pCapture->SetDrawWnd(GetDlgItem(IDC_PREVIEW)->GetSafeHwnd());
		hr = g_pCapture->StartCamera(pActivate, params);
	}

	if (SUCCEEDED(hr))
	{
		UpdateUI();
	}


	SafeRelease(&pActivate);

	if (FAILED(hr))
	{
		NotifyError(L"Error starting capture.", hr);
	}
}

void CTestCameraDlg::NotifyError(LPCWSTR info, HRESULT hr)
{
	const size_t MESSAGE_LEN = 512;
	WCHAR message[MESSAGE_LEN];

	swprintf_s(message, MESSAGE_LEN, L"%s (HRESULT = 0x%X)", info, hr);

	MessageBox(message, NULL, MB_OK | MB_ICONERROR);
}

HRESULT CTestCameraDlg::GetSelectedDevice(IMFActivate ** ppActivate)
{
	HWND hDeviceList = ::GetDlgItem(m_hWnd, IDC_DEVICE_LIST);

	// First get the index of the selected item in the combo box.
	int iListIndex = ComboBox_GetCurSel(hDeviceList);

	if (iListIndex == CB_ERR)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// Now find the index of the device within the device list.
	//
	// This index is stored as item data in the combo box, so that 
	// the order of the combo box items does not need to match the
	// order of the device list.

	LRESULT iDeviceIndex = ComboBox_GetItemData(hDeviceList, iListIndex);

	if (iDeviceIndex == CB_ERR)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// Now create the media source.

	return g_devices.GetDevice((UINT32)iDeviceIndex, ppActivate);
}

void CTestCameraDlg::OnSelectEncodingType(FileContainer file)
{
	WCHAR pszFile[MAX_PATH] = { 0 };

	HWND hEdit = ::GetDlgItem(m_hWnd, IDC_OUTPUT_FILE);

	::GetWindowTextW(hEdit, pszFile, MAX_PATH);

	switch (file)
	{
	case FileContainer_MP4:

		PathRenameExtension(pszFile, L".mp4");
		break;

	case FileContainer_WMV:
		PathRenameExtension(pszFile, L".wmv");
		break;

	default:
		assert(false);
		break;
	}

	::SetWindowTextW(hEdit, pszFile);
}

void CTestCameraDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	OnCloseDialog();
	CDialogEx::OnOK();
}


void CTestCameraDlg::OnBnClickedCancel()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	OnCloseDialog();
	CDialogEx::OnCancel();
}


void CTestCameraDlg::OnBnClickedCapture()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if (g_pCapture && g_pCapture->IsRecording())
	{
		StopCapture();
	}
	else
	{
		StartCapture();
	}
}


void CTestCameraDlg::OnCbnSelchangeDeviceList()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	// Now check if the current video capture device was lost.

	//StopCapture();
	QueryMediaTypeList();
}


void CTestCameraDlg::OnBnClickedCaptureMp4()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	OnSelectEncodingType(FileContainer_MP4);
}


void CTestCameraDlg::OnBnClickedCaptureWmv()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	OnSelectEncodingType(FileContainer_WMV);
}

void CTestCameraDlg::QueryMediaTypeList()
{
	HRESULT hr = S_OK;
	IMFActivate *pActivate = NULL;

	// Create the media source for the capture device.

	if (SUCCEEDED(hr))
	{
		hr = GetSelectedDevice(&pActivate);
	}

	// Start capturing.

	if (SUCCEEDED(hr))
	{
		hr = CCapture::CreateInstance(m_hWnd, &g_pCapture);
	}

	vector <wstring> arrList;

	if (SUCCEEDED(hr))
	{
		hr = g_pCapture->QueryMediaType(pActivate, arrList);
	}

	if (SUCCEEDED(hr))
	{
		m_oMediaType.ResetContent();
		for (size_t i = 0; i < arrList.size(); ++i)
		{
			int nIndex = m_oMediaType.AddString(arrList[i].c_str());
			m_oMediaType.SetItemData(nIndex, i);
		}
		if (m_oMediaType.GetCount() > 0)
		{
			m_oMediaType.SetCurSel(0);
		}
	}

	SafeRelease(&pActivate);
	SafeRelease(&g_pCapture);

	WCHAR *szFriendlyName = NULL;

	// NOTE: Updating the device list releases the existing IMFActivate 
	// pointers. This ensures that the current instance of the video capture 
	// source is released.
	g_devices.Clear();
	hr = g_devices.EnumerateDevices();
		
	if (FAILED(hr))
	{
		NotifyError(L"Error stopping capture. File might be corrupt.", hr);
	}
done:

	return;
}


void CTestCameraDlg::OnCbnSelchangeMediaType()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	StopCamra();
	StartCamera();
	UpdateVideoProcParams();
}

inline void SetAmpSlider(IMFActivate * pActive, CCapture * pCapture, CSliderCtrl & slider, CButton & btAuto, VideoProcAmpProperty eProperty)
{
	long lMax, lVal;
	pCapture->GetVideoProcAmpInfo(pActive, VideoProcAmp_Brightness, lMax, lVal);

	if (lVal == -1)
	{
		slider.SetPos(0);
		slider.EnableWindow(FALSE);
		btAuto.SetCheck(BST_CHECKED);
	}
	else
	{
		slider.SetPos(lVal);
		slider.EnableWindow(TRUE);
		btAuto.SetCheck(BST_UNCHECKED);
	}

	slider.SetSelection(0, lMax);
}

void CTestCameraDlg::UpdateVideoProcParams()
{
	long lMax, lVal;
	IMFActivate * pActive = NULL;
	auto hr = GetSelectedDevice(&pActive);
	if (SUCCEEDED(hr))
	{
		SetAmpSlider(pActive, g_pCapture, m_sliderBright, m_btAutoBright, VideoProcAmp_Brightness);
	}
}


void CTestCameraDlg::OnTRBNThumbPosChangingSliderBright(NMHDR *pNMHDR, LRESULT *pResult)
{
	// �˹���Ҫ�� Windows Vista ����߰汾��
	// _WIN32_WINNT ���ű��� >= 0x0600��
	NMTRBTHUMBPOSCHANGING *pNMTPC = reinterpret_cast<NMTRBTHUMBPOSCHANGING *>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;


}


void CTestCameraDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);

	IMFActivate * pActive = NULL;
	HRESULT hr = GetSelectedDevice(&pActive);

	if (SUCCEEDED(hr))
	{
		g_pCapture->SetVideoProcAmpVal(pActive, VideoProcAmp_Brightness, m_sliderBright.GetPos());
	}
}
