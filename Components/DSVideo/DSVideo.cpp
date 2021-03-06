                              //MyVideo.cpp//                                
//////////////////////////////////////////////////////////////////////////////////
//																				//
// Author:  Simeon Kosnitsky													//
//          skosnits@gmail.com													//
//																				//
// License:																		//
//     This software is released into the public domain.  You are free to use	//
//     it in any way you like, except that you may not sell this source code.	//
//																				//
//     This software is provided "as is" with no expressed or implied warranty.	//
//     I accept no liability for any damage or loss of business that this		//
//     software may cause.														//
//																				//
//////////////////////////////////////////////////////////////////////////////////

#include "DSVideo.h"
#include <Dvdmedia.h>

static CLSID CLSID_TransNull32 = {0x1916c5c7, 0x2aa, 0x415f, 0x89, 0xf, 0x76, 0xd9, 0x4c, 0x85, 0xaa, 0xf1};
static CLSID CLSID_DivX = {0x78766964, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
static CLSID CLSID_ffdshow = {0x04FE9017, 0xF873, 0x410E, 0x87, 0x1E, 0xAB, 0x91 , 0x66, 0x1A, 0x4E, 0xF7};
static CLSID CLSID_NullRenderer = { 0xc1f400a4, 0x3f08, 0x11d3, 0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37 };

/////////////////////////////////////////////////////////////////////////////

string WCSToStr(WCHAR *wstr)
{
	static char str[100];

	wcstombs( str, wstr, 100 );

	return string(str);
}

/////////////////////////////////////////////////////////////////////////////

LPCWSTR StringToLPCWSTR(string csStr)
{
	static WCHAR wName[500]; 

	int len = MultiByteToWideChar( CP_ACP, 0, csStr.c_str(), -1, NULL, 0);

	MultiByteToWideChar( CP_ACP, 0, csStr.c_str(), -1, wName, len);

	wName[len] = L'\0';

	return wName;
}

/////////////////////////////////////////////////////////////////////////////

string IntToCStr(int n)
{
	char str[100];

	_itoa(n, str, 10);

	return string(str);
}

/////////////////////////////////////////////////////////////////////////////

CTransNull32::CTransNull32( int **ppBuffer, s64 *pST,
                            bool *pImageGeted, IMediaControl *pMC,
                            bool *pIsSetNullRender, LPUNKNOWN punk, HRESULT *phr )
    : CTransInPlaceFilter(TEXT("TransNull32"), punk, CLSID_TransNull32, phr)
{
    m_ppBuffer = ppBuffer;
    m_pST = pST;
    m_pImageGeted = pImageGeted;
    m_pMC = pMC;
    m_pIsSetNullRender = pIsSetNullRender;

    *m_pImageGeted = true;
	m_TriengToGetImage = false;
    *m_pST = 0;

    m_w = 0;
    m_h = 0;
 
    m_ft = 1; //format type

	m_blnReInit = 1;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CTransNull32::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    return CBaseFilter::NonDelegatingQueryInterface(riid,ppv);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CTransNull32::CheckInputType(const CMediaType *mtIn)
{
    CheckPointer(mtIn,E_POINTER);

	if (mtIn == NULL)
	{
		return E_INVALIDARG;
	}

    if( *mtIn->Type( ) != MEDIATYPE_Video )
    {
        return E_INVALIDARG;
    }

    if ( (*mtIn->FormatType() != FORMAT_VideoInfo) &&
         (*mtIn->FormatType() != FORMAT_VideoInfo2) )
    {
        return E_INVALIDARG;
    }

    if( *mtIn->Subtype( ) != MEDIASUBTYPE_RGB32 )
    {
        return E_INVALIDARG;
    }

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CTransNull32::Transform(IMediaSample *pSample)
{
    HRESULT hr;
    VIDEOINFOHEADER  *pVi1;
    VIDEOINFOHEADER2 *pVi2;
    s64 StartTime, StopTime;
    BYTE *pBuffer;
    int *pIntBuffer;
    long BufferLen;
    int w, h, x, y, i, j, dj;
    AM_MEDIA_TYPE mt, *pmt;
    IPin *pIn = NULL;

	if ( (*m_pIsSetNullRender == false) || (*m_pImageGeted == false) )
    {	
        hr = pSample->GetPointer(&pBuffer);
        BufferLen = pSample->GetSize();
        pIntBuffer = (int*)pBuffer;

		if (m_blnReInit == 1)
		{
			hr = m_pInput->ConnectedTo(&pIn);
			hr = pIn->ConnectionMediaType(&mt);
			pmt = &mt;
	        
			if (SUCCEEDED(hr))
			{
				if (pmt->formattype == FORMAT_VideoInfo2)
				{
					m_ft = 2;
					pVi2 = (VIDEOINFOHEADER2*)pmt->pbFormat;
					m_w = pVi2->bmiHeader.biWidth;
					m_h = pVi2->bmiHeader.biHeight;
				}
				else
				{
					m_ft = 1;
					pVi1 = (VIDEOINFOHEADER*)pmt->pbFormat;
					m_w = pVi1->bmiHeader.biWidth;
					m_h = pVi1->bmiHeader.biHeight;
				}
			}			

			if (*m_ppBuffer == NULL)
			{
				*m_ppBuffer = new int[BufferLen/sizeof(int)];
			}

			m_blnReInit = 0;
		}
                

        if (m_h < 0)
        {
            memcpy(*m_ppBuffer, pBuffer, BufferLen);
        }
        else
        {
            w = m_w;
	        h = m_h;

	        dj = -2*w;

	        i = 0;
	        j = (h-1)*w;

	        for(y=0; y<h; y++)
	        {
		        for(x=0; x<w; x++)
		        {
			        (*m_ppBuffer)[j] = pIntBuffer[i];
			        i++;
			        j++;
		        }		        
		        j += dj; 
            }
        }

        hr = pSample->GetTime(&StartTime, &StopTime);
        StartTime += m_pInput->CurrentStartTime();        
        *m_pST = StartTime;

		if (*m_pImageGeted == false)
		{
			*m_pImageGeted = true;
			m_pMC->Pause();
		}
    }

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT DSVideo::GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
    if (!pFilter || !ppPin) return E_POINTER;
    *ppPin = NULL;

    IEnumPins  *pEnum;
    IPin       *pPin;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr)) return hr;
		
	while (pEnum->Next(1, &pPin, 0) == S_OK) // SUCCEEDED is incorrect here
    {
        PIN_DIRECTION PinDirThis;
        hr = pPin->QueryDirection(&PinDirThis);
        if (FAILED(hr)) break;

        if (PinDir == PinDirThis)
        {
            pEnum->Release();
            *ppPin = pPin;
            return S_OK;
        }
        pPin->Release();
    }
    pEnum->Release();
    return E_FAIL;  
}

/////////////////////////////////////////////////////////////////////////////

HRESULT DSVideo::ConnectFilters(IGraphBuilder *pGraph,IBaseFilter *pFirst,IBaseFilter *pSecond)
{
    HRESULT hr;

	if (!pGraph || !pFirst || !pSecond) return E_POINTER;

    IPin *pOut = NULL, *pIn = NULL;
    
	hr = pFirst->FindPin(L"Raw Video 1",&pOut);
	if (FAILED(hr)) hr = pFirst->FindPin(L"Video",&pOut);
	if (FAILED(hr)) hr = pFirst->FindPin(L"Video 0",&pOut);
	if (FAILED(hr)) hr = pFirst->FindPin(L"Stream 2",&pOut);
	if (FAILED(hr)) hr = GetPin(pFirst, PINDIR_OUTPUT, &pOut);
    
	if (FAILED(hr)) return hr;

    hr = GetPin(pSecond, PINDIR_INPUT, &pIn);

	if (FAILED(hr)) 
    {
        pOut->Release();
        return E_FAIL;
    }

    hr = pGraph->Connect(pOut, pIn);
    pIn->Release();
    pOut->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

DSVideo::DSVideo()
{
	m_Inited=false;
	m_pGB=NULL;
	m_pMC=NULL;
	m_pME=NULL;
	m_pMS=NULL;
	m_pVW=NULL;
	m_pBV=NULL;
	m_pBA=NULL;
	m_pMF=NULL;
	m_pDecoder=NULL;
	//m_pGrabber=NULL;
	m_pSourceFilter=NULL;
	m_pSampleGrabberFilter=NULL;
    m_pTransNull32Filter=NULL;
	m_pVideoRenderFilter=NULL;
	m_pBuilder=NULL;
	//m_pSGCallback=NULL;
	m_IsMSSuported = true;
	m_IsSetNullRender = false;
    
    m_pBuffer=NULL;
	m_st = 0;

	m_type = 0;
}

/////////////////////////////////////////////////////////////////////////////

DSVideo::~DSVideo()
{
	if (m_Inited) CleanUp();
}

/////////////////////////////////////////////////////////////////////////////

bool DSVideo::OpenMovie(string csMovieName, void *pVideoWindow, int type)
{ 	
	HRESULT hr;
	string Str;
	vector<CLSID> cls;
	vector<string> fnames;
    AM_MEDIA_TYPE mt;	
    IPin *pIn;
	ULONG res;
	int i;
	
	try
	{
		if (m_type == -3)
		{
			m_type = 3;
		}
		else
		{
			m_type = 2;
		}

		m_MovieName = csMovieName;

		m_log += "OpenMovieNormally(...)\n";

		if (m_Inited)
		{
			hr = CleanUp();
			if (FAILED(hr)) { return false; }
		}

		m_log += "CoInitialize(NULL)\n";
		hr = CoInitialize(NULL);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "CoCreateInstance(CLSID_FilterGraph,...)\n";
		hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
			IID_IGraphBuilder, (void **)&m_pGB);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "CoCreateInstance(CLSID_CaptureGraphBuilder2,...)\n";
		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL,
			CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2,
			(void **)&m_pBuilder);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "m_pBuilder->SetFiltergraph(m_pGB)\n";
		hr = m_pBuilder->SetFiltergraph(m_pGB);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "m_pGB->QueryInterface(IID_IMediaControl,...)\n";
		hr = m_pGB->QueryInterface(IID_IMediaControl, (void **)&m_pMC);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "m_pGB->QueryInterface(IID_IMediaEventEx,...)\n";
		hr = m_pGB->QueryInterface(IID_IMediaEventEx, (void **)&m_pME);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "m_pGB->QueryInterface(IID_IMediaSeeking,...)\n";
		hr = m_pGB->QueryInterface(IID_IMediaSeeking, (void **)&m_pMS);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "CoCreateInstance(CLSID_VideoMixingRenderer,...)\n";
		hr = CoCreateInstance(CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC_SERVER,
			IID_IBaseFilter, (void**)&m_pVideoRenderFilter);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "new CTransNull32(...)\n";
		m_pTransNull32 = new CTransNull32(&m_pBuffer, &m_st,
			&m_ImageGeted, m_pMC,
			&m_IsSetNullRender, NULL, &hr);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "m_pTransNull32->QueryInterface(IID_IBaseFilter,...)\n";
		hr = m_pTransNull32->QueryInterface(IID_IBaseFilter,
			reinterpret_cast<void**>(&m_pTransNull32Filter));
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "m_pGB->AddSourceFilter(...,m_pSourceFilter)\n";
		hr = m_pGB->AddSourceFilter(StringToLPCWSTR(m_MovieName), L"Source1", &m_pSourceFilter);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "m_pGB->AddFilter(m_pTransNull32Filter,...)\n";
		hr = m_pGB->AddFilter(m_pTransNull32Filter, L"MyColorSpaceConverter");
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "m_pGB->AddFilter(m_pVideoRenderFilter,...)\n";
		hr = m_pGB->AddFilter(m_pVideoRenderFilter, L"Video Renderer");
		if (FAILED(hr)) { CleanUp(); return false; }

		try
		{
			m_log += "m_pBuilder->RenderStream(0, 0, m_pSourceFilter, m_pTransNull32Filter, m_pVideoRenderFilter)\n";
			hr = m_pBuilder->RenderStream(0, 0, m_pSourceFilter, m_pTransNull32Filter, m_pVideoRenderFilter);
		}
		catch (...)
		{
			m_log += "got exception...\n";
			hr = E_FAIL;
		}			
		
		if (FAILED(hr))
		{
			m_log += "Failed to RenderStream.\n";

			try
			{
				m_log += "m_pBuilder->RenderStream(0, 0, m_pSourceFilter, 0, m_pTransNull32Filter)\n";
				hr = m_pBuilder->RenderStream(0, 0, m_pSourceFilter, 0, m_pTransNull32Filter);
			}
			catch (...)
			{
				m_log += "got exception...\n";
				hr = E_FAIL;
			}

			if (SUCCEEDED(hr))
			{
				m_log += "Passed to RenderStream.\n";

				try
				{
					m_log += "m_pBuilder->RenderStream(0, 0, m_pTransNull32Filter, 0, m_pVideoRenderFilter)\n";
					hr = m_pBuilder->RenderStream(0, 0, m_pTransNull32Filter, 0, m_pVideoRenderFilter);
				}
				catch (...)
				{
					m_log += "got exception...\n";
					hr = E_FAIL;
				}
			}
		}

		if (FAILED(hr))
		{
			m_log += "Failed to RenderStream.\n";

			m_log += "Trying to use alternative decoders...\n";

			cls.push_back(CLSID_AVIDec);
			fnames.push_back("AVI Decoder");

			cls.push_back(CLSID_ffdshow);
			fnames.push_back("ffdshow MPEG-4 Video Decoder");

			cls.push_back(CLSID_DivX);
			fnames.push_back("DivX Decoder Filter");

			for (i = 0; i < (int)cls.size(); i++)
			{
				m_log += "CoCreateInstance(" + fnames[i] + ")\n";

				hr = CoCreateInstance(cls[i], NULL, CLSCTX_INPROC_SERVER,
					IID_IBaseFilter, (void**)&m_pDecoder);

				if (SUCCEEDED(hr))
				{
					m_log += "Passed CoCreateInstance\n";

					m_log += "AddFilter(m_pDecoder,...)\n";
					hr = m_pGB->AddFilter(m_pDecoder, L"Video Decoder");

					if (SUCCEEDED(hr))
					{
						m_log += "Passed AddFilter\n";
						try
						{
							m_log += "ConnectFilters(m_pGB, m_pSourceFilter, m_pDecoder)\n";
							hr = ConnectFilters(m_pGB, m_pSourceFilter, m_pDecoder);
						}
						catch (...)
						{
							m_log += "got exception...\n";
							hr = E_FAIL;
						}

						if (SUCCEEDED(hr))
						{
							m_log += "ConnectFilters(m_pGB, m_pDecoder, m_pTransNull32Filter)\n";
							hr = ConnectFilters(m_pGB, m_pDecoder, m_pTransNull32Filter);
						}

						if (FAILED(hr))
						{
							m_log += "Failed ConnectFilters\n";
							hr = m_pGB->RemoveFilter(m_pDecoder);
							res = m_pDecoder->Release();
							m_pDecoder = NULL;
							hr = E_FAIL;
						}
						else
						{
							break;
						}
					}
				}
			}

			if (FAILED(hr))
			{
				m_log += "[DSHOW] Can't render stream SourceFilter to TransNull32Filter.\n";
				MessageBox(NULL, "[DSHOW] Can't render stream SourceFilter to TransNull32Filter.", "ERROR MESSAGE", MB_ICONERROR);
				CleanUp();
				return false;
			}

			if (SUCCEEDED(hr))
			{
				m_log += "m_pBuilder->RenderStream(0, 0, m_pTransNull32Filter, 0, m_pVideoRenderFilter)\n";
				hr = m_pBuilder->RenderStream(0, 0, m_pTransNull32Filter, 0, m_pVideoRenderFilter);
			}

			if (FAILED(hr))
			{
				m_log += "[DSHOW] Can't render stream TransNull32Filter to VideoRenderFilter.\n";
				MessageBox(NULL, "[DSHOW] Can't render stream TransNull32Filter to VideoRenderFilter.", "ERROR MESSAGE", MB_ICONERROR);
				CleanUp();
				return false;
			}
		}

		m_log += "m_pGB->QueryInterface(IID_IMediaFilter,...)\n";
		hr = m_pGB->QueryInterface(IID_IMediaFilter, (void **)&m_pMF);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "m_pGB->QueryInterface(IID_IVideoWindow,...)\n";
		hr = m_pGB->QueryInterface(IID_IVideoWindow, (void **)&m_pVW);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "m_pVW->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS)\n";
		hr = m_pVW->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
		if (FAILED(hr)) { m_log += "failed m_pVW->put_WindowStyle\n"; }

		m_log += "m_pVW->put_Owner(*((OAHWND*)pVideoWindow->GetHandle()))\n";
		hr = m_pVW->put_Owner(*((OAHWND*)pVideoWindow));
		if (FAILED(hr)) { m_log += "failed m_pVW->put_Owner\n"; }

		m_log += "m_pVW->put_MessageDrain(*((OAHWND*)pVideoWindow->GetHandle()))\n";
		hr = m_pVW->put_MessageDrain(*((OAHWND*)pVideoWindow));
		if (FAILED(hr)) { m_log += "failed m_pVW->put_MessageDrain\n"; }

		m_log += "m_pMS->GetStopPosition(&m_Duration)\n";
		hr = m_pMS->GetStopPosition(&m_Duration);
		m_Duration /= 10000;
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "GetPin(m_pTransNull32Filter, PINDIR_INPUT, &pIn)\n";
		hr = GetPin(m_pTransNull32Filter, PINDIR_INPUT, &pIn);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_log += "pIn->ConnectionMediaType(&mt)\n";
		hr = pIn->ConnectionMediaType(&mt);
		if (FAILED(hr)) { CleanUp(); return false; }

		if (mt.formattype == FORMAT_VideoInfo)
		{
			m_log += "mt.formattype == FORMAT_VideoInfo\n";

			VIDEOINFOHEADER  *pVi = (VIDEOINFOHEADER*)mt.pbFormat;

			m_Width = abs(pVi->bmiHeader.biWidth);
			m_Height = abs(pVi->bmiHeader.biHeight);
		}
		else if (mt.formattype == FORMAT_VideoInfo2)
		{
			m_log += "mt.formattype == FORMAT_VideoInfo2\n";

			VIDEOINFOHEADER2 *pVi = (VIDEOINFOHEADER2*)mt.pbFormat;

			m_Width = abs(pVi->bmiHeader.biWidth);
			m_Height = abs(pVi->bmiHeader.biHeight);
		}
		else
		{
			m_log += "Unsupported video format\n";
			CleanUp();
			return false;
		}

		m_log += "m_pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME)\n";
		hr = m_pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
		if (FAILED(hr)) { CleanUp(); return false; }

		m_pTransNull32->m_blnReInit = 1;

		m_Inited = true;
	}
	catch (...)
	{
		m_log += "got global exception...\n";
		CleanUp();
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////

bool DSVideo::SetVideoWindowPlacement(void *pVideoWindow)
{
	HRESULT hr;
	bool result = false;
	CLSID classID;
	IPin *pOut, *pIn;

	while(1)
	{
		hr = m_pVideoRenderFilter->GetClassID(&classID);
		if (FAILED(hr)) break;

		pIn = NULL;
		hr = GetPin(m_pVideoRenderFilter, PINDIR_INPUT, &pIn);
		if (FAILED(hr)) break;
	
		pOut = NULL;
		hr = pIn->ConnectedTo(&pOut);
		if (FAILED(hr)) break;

		hr = m_pVW->put_Visible(false);

		hr = m_pVW->put_MessageDrain(NULL);
	
		hr = m_pVW->put_Owner(NULL);

		m_pVW->Release();	
		m_pVW = NULL;

		hr = pIn->Disconnect();
	
		hr = pOut->Disconnect();

		hr = m_pGB->RemoveFilter(m_pVideoRenderFilter);
		if (FAILED(hr)) break;

		m_pVideoRenderFilter->Release();
		
		m_pVideoRenderFilter = NULL;
		hr = CoCreateInstance(classID, NULL, CLSCTX_INPROC_SERVER,
								IID_IBaseFilter,(void**)&m_pVideoRenderFilter);
		if (FAILED(hr)) break;

		hr = m_pGB->AddFilter(m_pVideoRenderFilter, L"Video Render");
		if (FAILED(hr)) break;
		
		pIn = NULL;
		hr = GetPin(m_pVideoRenderFilter, PINDIR_INPUT, &pIn);
		if (FAILED(hr)) break;

		hr = m_pGB->ConnectDirect(pOut, pIn, NULL);
		if (FAILED(hr)) break;

		break;
	}
	
	hr = m_pGB->QueryInterface(IID_IVideoWindow,(void **)&m_pVW);

	hr = m_pVW->put_Visible(true);

	hr = m_pVW->put_Owner(*((OAHWND*)pVideoWindow));

	hr = m_pVW->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
	
	hr = m_pVW->put_MessageDrain(*((OAHWND*)pVideoWindow));

	if (SUCCEEDED(hr)) result = true;

	return result;
}

IBaseFilter* DSVideo::GetDecoder()
{
	HRESULT hr;
	IPin *pOut, *pIn;
	PIN_INFO PinInfo;
	IBaseFilter* pFilter = NULL;

	while(1)
	{
		if (m_pSampleGrabberFilter == NULL) break;

		pIn = NULL;
		hr = GetPin(m_pSampleGrabberFilter, PINDIR_INPUT, &pIn);
		if (FAILED(hr)) break;

		pOut = NULL;
		hr = pIn->ConnectedTo(&pOut);
		if (FAILED(hr)) break;

		hr = pOut->QueryPinInfo(&PinInfo);
		if (FAILED(hr)) break;

		pFilter = PinInfo.pFilter;

		break;
	}

	return pFilter;
}

IBaseFilter* DSVideo::GetSourceFilter()
{
	HRESULT hr;
	IPin *pOut, *pIn;
	PIN_INFO PinInfo;
	IBaseFilter* pFilter = NULL;

	while(1)
	{
		if (m_pSampleGrabberFilter == NULL) break;

		pIn = NULL;
		hr = GetPin(m_pSampleGrabberFilter, PINDIR_INPUT, &pIn);
		
		while (SUCCEEDED(hr))
		{
			pOut = NULL;
			hr = pIn->ConnectedTo(&pOut);
			if (FAILED(hr)) { pFilter = NULL; break; }

			hr = pOut->QueryPinInfo(&PinInfo);
			if (FAILED(hr)) { pFilter = NULL; break; }

			pFilter = PinInfo.pFilter;

			pIn = NULL;
			hr = GetPin(pFilter, PINDIR_INPUT, &pIn);
		}

		break;
	}

	return pFilter;
}

/////////////////////////////////////////////////////////////////////////////

bool DSVideo::CloseMovie()
{
	return true;
}

/////////////////////////////////////////////////////////////////////////////

bool DSVideo::SetNullRender()
{
	HRESULT hr;

	IPin *pOutGB, *pInVR;

	if (m_Inited == false) return false;
	
	hr = m_pVW->put_Visible(false);	

	if (m_type != 3)
	{
		hr = m_pVW->put_MessageDrain(NULL);
	
		hr = m_pVW->put_Owner(NULL);

		hr = m_pGB->RemoveFilter(m_pVideoRenderFilter);
		if (FAILED(hr)) { CleanUp(); return false; }

		if (m_pSampleGrabberFilter != NULL)
		{
			hr = GetPin(m_pSampleGrabberFilter, PINDIR_OUTPUT, &pOutGB);
			if (FAILED(hr)) { CleanUp(); return false; }
		}
		else
		{
			hr = GetPin(m_pTransNull32Filter, PINDIR_OUTPUT, &pOutGB);
			if (FAILED(hr)) { CleanUp(); return false; }
		}

		m_pVideoRenderFilter = NULL;
		hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
								IID_IBaseFilter,(void**)&m_pVideoRenderFilter);

		hr = m_pGB->AddFilter(m_pVideoRenderFilter, L"Video Render");

		hr = GetPin(m_pVideoRenderFilter, PINDIR_INPUT, &pInVR);
		if (FAILED(hr)) { CleanUp(); return false; }

		hr = pInVR->Disconnect();

		hr = pOutGB->Disconnect();

		hr = m_pGB->Connect(pOutGB, pInVR);
		if (FAILED(hr)) { CleanUp(); return false; }

		if (m_pTransNull32Filter != NULL)
		{
			m_pTransNull32->m_blnReInit = 1;
		}
	}
	else
	{
		hr = m_pVW->SetWindowPosition(-(100+m_Width), -(100+m_Height), m_Width, m_Height);
	}

	hr = m_pMF->SetSyncSource(NULL);

	m_IsMSSuported = false;

	m_IsSetNullRender = true;

	return true;
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::SetPos(s64 Pos)
{
	HRESULT hr;
	s64 endPos;
	long evCode;
	
	Pos *= 10000;
	endPos = m_Duration*10000;

	m_ImageGeted = true;

	m_pMS->SetPositions(&Pos,AM_SEEKING_AbsolutePositioning,&endPos,AM_SEEKING_AbsolutePositioning);
	m_st = Pos;  

	m_ImageGeted = false;
	hr = m_pMC->Run();
	hr = m_pME->WaitForCompletion(300, &evCode);

	if (m_ImageGeted != true)
	{
		m_pMC->Pause();
	}
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::SetPos(double pos)
{
	HRESULT hr;
	s64 Pos, endPos;
	long evCode;

	endPos = m_Duration*10000;
	Pos = (s64)(pos*10000000.0);

	m_ImageGeted = true;

	m_pMS->SetPositions(&Pos,AM_SEEKING_AbsolutePositioning,&endPos,AM_SEEKING_AbsolutePositioning);
	m_st = Pos;  

	m_ImageGeted = false;
	hr = m_pMC->Run();
	hr = m_pME->WaitForCompletion(300, &evCode);

	if (m_ImageGeted != true)
	{
		m_pMC->Pause();
	}
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::SetPosFast(s64 Pos)
{
	HRESULT hr;
	s64 endPos;
	long evCode;
	
	Pos *= 10000;
	endPos = m_Duration*10000;

	m_ImageGeted = true;

	hr = m_pMS->SetPositions(&Pos,AM_SEEKING_AbsolutePositioning,&endPos,AM_SEEKING_AbsolutePositioning);
	m_st = Pos;  

	hr = m_pME->WaitForCompletion(300, &evCode);
	
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::Stop()
{
	if(m_pMC != NULL)
	{
		HRESULT hr = m_pMC->Pause();
		this->SetPos((s64)0);
		m_pMC->Stop();
	}
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::StopFast()
{
	this->SetPos((s64)0);
	m_pMC->Stop();
}

/////////////////////////////////////////////////////////////////////////////

HRESULT DSVideo::CleanUp()
{
	HRESULT hr = S_OK;
	string log;
	int i, max_n = 100;
	IPin *pOut, *pIn;
	fstream fout;
	string fname;

	m_log += "Performing CleanUp...\n";

	try
	{
		fname = m_Dir + string("/clean_video.log");
		fout.open(fname.c_str(), ios_base::out | ios_base::app);
		fout << "";
		fout.close();

		this->Stop();

		log += "PASS: m_pME->WaitForCompletion(...)\n";

		if (m_pVW != NULL)
		{
			hr = m_pVW->put_Visible(OAFALSE);
			hr = m_pVW->put_MessageDrain(NULL);
			hr = m_pVW->put_Owner(NULL);
		}

		if (m_pDecoder == NULL)
		{
			m_pDecoder = GetDecoder();
		}

		if (m_pDecoder != NULL)	log += "PASS: GetDecoder()\n";
		else log += "FAIL: GetDecoder()\n";

		if (m_pVideoRenderFilter != NULL)
		{
			pOut = NULL;
			hr = GetPin(m_pVideoRenderFilter, PINDIR_OUTPUT, &pOut);
			pIn = NULL;
			hr = GetPin(m_pVideoRenderFilter, PINDIR_INPUT, &pIn);

			if (pOut != NULL) hr = pOut->Disconnect();
			if (pIn != NULL) hr = pIn->Disconnect();

			hr = m_pGB->RemoveFilter(m_pVideoRenderFilter);

			if (SUCCEEDED(hr)) log += "PASS: Delete VideoRenderFilter\n";
			else log += "FAIL: Delete VideoRenderFilter\n";
		}

		if (m_pSampleGrabberFilter != NULL)
		{
			hr = m_pGB->RemoveFilter(m_pSampleGrabberFilter);

			if (SUCCEEDED(hr)) log += "PASS: Delete SampleGrabberFilter\n";
			else log += "FAIL: Delete SampleGrabberFilter\n";
		}

		if (m_pDecoder != NULL)
		{
			hr = m_pGB->RemoveFilter(m_pDecoder);

			if (SUCCEEDED(hr)) log += "PASS: Delete Decoder\n";
			else log += "FAIL: Delete Decoder\n";
		}

		if (m_pMC != NULL) i = m_pMC->Release();
		m_pMC = NULL;

		if (m_pME != NULL) i = m_pME->Release();
		m_pME = NULL;

		if (m_pMS != NULL) i = m_pMS->Release();
		m_pMS = NULL;

		if (m_pMF != NULL) i = m_pMF->Release();
		m_pMF = NULL;

		if (m_pVW != NULL) i = m_pVW->Release();
		m_pVW = NULL;

		if (m_pBV != NULL) i = m_pBV->Release();
		m_pBV = NULL;

		if (m_pBA != NULL) i = m_pBA->Release();
		m_pBA = NULL;

		if (m_pDecoder != NULL) i = m_pDecoder->Release();
		m_pDecoder = NULL;

		if (m_pSampleGrabberFilter != NULL) i = m_pSampleGrabberFilter->Release();
		m_pSampleGrabberFilter = NULL;

		if (m_pTransNull32Filter != NULL) i = m_pTransNull32Filter->Release();
		m_pTransNull32Filter = NULL;

		if (m_pSourceFilter != NULL) i = m_pSourceFilter->Release();
		m_pSourceFilter = NULL;

		if (m_pVideoRenderFilter != NULL) i = m_pVideoRenderFilter->Release();
		m_pVideoRenderFilter = NULL;

		if (m_pGB != NULL) i = m_pGB->Release();
		m_pGB = NULL;

		if (m_pBuilder != NULL) i = m_pBuilder->Release();
		m_pBuilder = NULL;

		CoUninitialize();

		if (m_pBuffer != NULL) delete[] m_pBuffer;
		m_pBuffer = NULL;

		m_Inited = false;

		m_IsMSSuported = true;
		m_st = 0;

		m_IsSetNullRender = false;

		fname = m_Dir + string("/clean_video.log");
		fout.open(fname.c_str(), ios::out);
		fout << log;
		fout.close();
	}
	catch (...)
	{
		m_log += "got exception on CleanUp...\n";
		return S_FALSE;
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::OneStep()
{
	long evCode;

	m_ImageGeted = false;
	m_pMC->Run();
	m_pME->WaitForCompletion(300, &evCode);

	if (m_ImageGeted != true)
	{
		m_pMC->Pause();
	}
}

/////////////////////////////////////////////////////////////////////////////

s64 DSVideo::OneStepWithTimeout()
{
	s64 CurPos, PrevPos, tmpPos;
	clock_t start_t, dt = 10000;
	long evCode;
	long min_frame_rate = 15;
	long ddt = (s64)120*(s64)10000;
	int bln = 0;

	PrevPos = this->GetPos();

	if (PrevPos == m_Duration)
	{
		CurPos = PrevPos;
	}
	else
	{
		m_ImageGeted = false;
		m_pMC->Run();
		m_pME->WaitForCompletion(500, &evCode);
		m_pMC->Pause();

		if ( (m_ImageGeted == false) && 
			  (PrevPos >= (m_Duration - ddt)) )
		{
			m_ImageGeted = true;
			CurPos = m_Duration;
		}
		else
		{
			if ( (m_ImageGeted == false) && 
				(PrevPos < (m_Duration - ddt*2)) )
			{
				start_t = clock();

				while ( ((clock() - start_t) < dt) && 
						(m_ImageGeted == false) )
				{
					this->SetPos(PrevPos);

					m_pMC->Run();
					m_pME->WaitForCompletion(1000, &evCode);
					m_pMC->Pause();
				}

				if (m_ImageGeted == false)
				{
					this->SetPos(PrevPos + ddt/3);
				}

				if (m_ImageGeted == false)
				{
					this->SetPos(PrevPos + ddt/2);
				}

				if (m_ImageGeted == false)
				{
					this->SetPos(PrevPos + (ddt*2)/3);
				}

				if (m_ImageGeted == false)
				{
					this->SetPos(PrevPos + ddt);
				}

				if (m_ImageGeted == false)
				{
					this->SetPos(PrevPos + (ddt*3)/2);
				}

				if (m_ImageGeted == false)
				{
					this->SetPos(PrevPos + ddt*2);
				}

				if (m_ImageGeted == false)
				{
					CurPos = PrevPos;
					bln = 1;
					//MessageBox(NULL, "Can'nt get new image.", "OneStepWithTimeout in DSVideo", MB_ICONERROR);
				}
			}
			
			if (bln == 0)
			{
				CurPos = tmpPos = this->GetPos();

				if ( (tmpPos == PrevPos) || ((tmpPos - PrevPos) > ddt) )
				{
					this->SetPos(PrevPos + ddt/3);
					CurPos = this->GetPos();

					if (CurPos == PrevPos)
					{
						this->SetPos(PrevPos + ddt/2);
						CurPos = this->GetPos();
					}

					if (CurPos == PrevPos)
					{
						this->SetPos(PrevPos + (ddt*2)/3);
						CurPos = this->GetPos();
					}

					if (CurPos == PrevPos)
					{
						this->SetPos(PrevPos + ddt);
						CurPos = this->GetPos();
					}

					if (CurPos == PrevPos)
					{
						this->SetPos(PrevPos + (ddt*3)/2);
						CurPos = this->GetPos();
					}

					if (CurPos == PrevPos)
					{
						this->SetPos(PrevPos + ddt*2);
						CurPos = this->GetPos();
					}

					if ((CurPos == PrevPos) && (tmpPos == PrevPos))
					{
						//MessageBox(NULL, "Can'nt get image with pos != prev_pos.", "OneStepWithTimeout in DSVideo", MB_ICONERROR);
					}
				}

				if (CurPos == PrevPos)
				{
					this->SetPos(tmpPos);
					CurPos = this->GetPos();

					if (CurPos == PrevPos)
					{
						CurPos = tmpPos;
					}
				}
			}
		}
	}    

	return CurPos;
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::ErrorMessage(string str)
{
	MessageBox(NULL, str.c_str(), "ERROR MESSAGE", MB_ICONERROR);
}

/////////////////////////////////////////////////////////////////////////////

s64 DSVideo::GetPos()
{
    s64 pos = -1;

	if ( m_IsMSSuported )
	{
		HRESULT hr = m_pMS->GetCurrentPosition(&pos);

		if (hr == E_NOTIMPL)
		{
			pos = m_st;
			m_IsMSSuported = false;
		}
	}
	else
	{
		pos = m_st;
	}
	
	if (pos > 0) pos /= 10000;
    return pos;
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::GetRGBImage(custom_buffer<int> &ImRGB, int xmin, int xmax, int ymin, int ymax)
{
	long evCode;
	int w, h, x, y, i, j, di;
	int *pBuffer = m_pBuffer;

	if (pBuffer == NULL)
	{
        m_pBuffer = new int[m_Width*m_Height];
        pBuffer = m_pBuffer;

        m_ImageGeted = false;
	    m_pMC->Run();
	    m_pME->WaitForCompletion(300, &evCode);		
	}

    w = xmax-xmin+1;
	h = ymax-ymin+1;

    if ((w == m_Width) && (h == m_Height))
    {
        memcpy(ImRGB.m_pData, pBuffer, m_Width*m_Height*sizeof(int));
    }
    else
    {    
	    di = m_Width-w;

	    i = ymin*m_Width + xmin;
	    j = 0;

	    for(y=0; y<h; y++)
	    {
		    for(x=0; x<w; x++)
		    {
			    ImRGB[j] = pBuffer[i];
			    i++;
			    j++;
		    }
		    i += di;
	    }
    }
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::SetImageGeted(bool ImageGeted)
{
	m_ImageGeted = ImageGeted;
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::RunWithTimeout(s64 timeout)
{
	long evCode;

	m_pMC->Run();
	m_pME->WaitForCompletion((long)timeout, &evCode);
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::Run()
{
	HRESULT hr;

	hr = m_pMC->Run();
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::Pause()
{
	m_pMC->Pause();
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::WaitForCompletion(s64 timeout)
{
	long evCode;

	m_pME->WaitForCompletion((long)timeout, &evCode);
}

/////////////////////////////////////////////////////////////////////////////

void DSVideo::SetVideoWindowPosition(int left, int top, int width, int height, void *dc)
{
	m_pVW->SetWindowPosition(left, top, width, height);
}

s64 DSVideo::PosToMilliSeconds(s64 pos)
{
	return (pos/(s64)10000);
}
