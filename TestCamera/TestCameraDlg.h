
// TestCameraDlg.h : 头文件
//

#pragma once
#include "capture.h"
#include "afxwin.h"
#include "afxcmn.h"

const UINT32 TARGET_BIT_RATE = 296 * 1000 * 1000;
enum FileContainer
{
	FileContainer_MP4 = IDC_CAPTURE_MP4,
	FileContainer_WMV = IDC_CAPTURE_WMV
};


// CTestCameraDlg 对话框
class CTestCameraDlg : public CDialogEx
{
// 构造
public:
	CTestCameraDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TESTCAMERA_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	HDEVNOTIFY  g_hdevnotify;
	DeviceList  g_devices;
	CCapture * g_pCapture;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	HRESULT UpdateDeviceList();
private:
	void UpdateUI();
	void EnableDialogControl(HWND hDlg, int nIDDlgItem, BOOL bEnable);
	void OnCloseDialog();

	void	UpdateParam(EncodingParameters & params);

	void    StopCapture();
	void    StartCapture();
	void    StopCamra();
	void    StartCamera();
	void NotifyError(LPCWSTR info, HRESULT hr);
	HRESULT GetSelectedDevice(IMFActivate ** ppActivate);
	void    OnSelectEncodingType(FileContainer file);
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedCapture();
	afx_msg void OnCbnSelchangeDeviceList();
	afx_msg void OnBnClickedCaptureMp4();
	afx_msg void OnBnClickedCaptureWmv();
	CComboBox m_oMediaType;

	void QueryMediaTypeList();
	CButton m_oHwEnc;
	afx_msg void OnCbnSelchangeMediaType();

	void UpdateVideoProcParams();
	CSliderCtrl m_sliderBright;
	CButton m_btAutoBright;
	afx_msg void OnTRBNThumbPosChangingSliderBright(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
