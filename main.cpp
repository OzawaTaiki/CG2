#include <Windows.h>
#include <string>
#include <format>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dxgidebug.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")

#include <dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")

#include "myLib/MyLib.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

#include <vector>

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _msg, WPARAM _wparam, LPARAM _lparam);

void Log(const std::string& message);
std::wstring ConvertString(const std::string& _str);
std::string ConvertString(const std::wstring& _str);

const int32_t  kClientWidth = 1280;
const int32_t  kClientHeight = 720;

IDxcBlob* ComplieShader(
	//Complierするshaderファイルへのパス
	const std::wstring& _filePath,
	//Compilerに使用するprofile
	const wchar_t* _profile,
	//初期化で生成したものを3つ
	IDxcUtils* _dxcUtils,
	IDxcCompiler3* _dxcCompiler,
	IDxcIncludeHandler* _includeHandler);

ID3D12Resource* CreateBufferResource(ID3D12Device* _device, size_t _sizeInBytes);

ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* _device, int32_t _width, int32_t _height);

ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* _device, D3D12_DESCRIPTOR_HEAP_TYPE _heapType, UINT _numDescriptors, bool _shaderVisible);

//textureデータを読む
DirectX::ScratchImage LoadTexture(const std::string& _filePath);

//DirectX13のtextureリソースを作る
ID3D12Resource* CreateTextureResource(ID3D12Device* _device, const DirectX::TexMetadata& _metadata);

//データを転送するUploadTextureData関数を作る
ID3D12Resource* UploadTextureData(ID3D12Resource* _texture, const DirectX::ScratchImage& _mipImages, ID3D12Device* _device, ID3D12GraphicsCommandList* _commandList);


D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* _descriptorHeap, uint32_t _descriptorSize, uint32_t _index);
D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* _descriptorHeap, uint32_t _descriptorSize, uint32_t _index);



struct VertexData
{
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;		//法線
};

struct Material
{
	Vector4 color;
	int32_t enabledLighthig;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct DirectionalLight
{
	Vector4 color;		//ライトの色
	Vector3 direction;	//ライトの向き
	float intensity;	//輝度
	int isHalf;
};

struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct  Object
{
	Material* materialData;
	TransformationMatrix* transformMat;
	VertexData* vertexData;
	ID3D12Resource* wvpResource;
	ID3D12Resource* indexResource;
	ID3D12Resource* vertexResource;
	ID3D12Resource* materialResource;
	float* visible;
	ID3D12Resource* texVisiblity;
	D3D12_VERTEX_BUFFER_VIEW indexBufferView;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	uint32_t vertexNum;


	~Object() {
		wvpResource->Release();
		vertexResource->Release();
		materialResource->Release();
		indexResource->Release();
	};
};

struct Texture
{
	ID3D12Resource* resource;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandlerCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandlerGPU;
	std::string name;
};

std::vector<Texture> textures;

/// <summary>
/// 読み込んだテクスチャを取り出す
/// </summary>
/// <param name="_textureHandle">テクスチャハンドル</param>
/// <returns>テクスチャデータ</returns>
D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(int _textureHandle);

/// <summary>
/// テクスチャを読み込む
/// </summary>
/// <param name="_filePath">ファイルパス</param>
/// <param name="_device">デバイス</param>
/// <param name="_commandList">コマンドリスト</param>
/// <param name="_srvDescriptorHeap">ｓｒｖディスクリプタヒープ</param>
/// <param name="_srvSize">srvのサイズ</param>
/// <returns>テクスチャの登録番号</returns>
int LoadTexture(const std::string& _filePath, ID3D12Device* _device, ID3D12GraphicsCommandList* _commandList, ID3D12DescriptorHeap* _srvDescriptorHeap, uint32_t _srvSize);

/// <summary>
/// 三角形のデータ作成
/// </summary>
/// <param name="_device">デバイス</param>
/// <param name="_obj">データ格納用Object変数</param>
void  MakeTriangleData(ID3D12Device* _device, Object* _obj);

/// <summary>
/// 球のデータ作成
/// </summary>
/// <param name="_device">デバイス</param>
/// <param name="_obj">データ格納用Object変数</param>
void MakeSphereData(ID3D12Device* _device, Object* _obj);

/// <summary>
/// スプライトのデータ作成
/// </summary>
/// <param name="_device">デバイス</param>
/// <param name="_obj">データ格納用Object変数</param>
void MakeSpriteData(ID3D12Device* _device, Object* _obj);


/// <summary>
/// スプライトのTransformationMatrixの計算
/// </summary>
/// <param name="_transform">トランスフォームデータ</param>
/// <returns>WVPt および WorldMat</returns>
TransformationMatrix CalculateSpriteWVPMat(const stTransform& _transform);

TransformationMatrix CalculateObjectWVPMat(const stTransform& _transform, const Matrix4x4& _VPmat);

/// <summary>
/// 三角形の描画
/// </summary>
/// <param name="_commandList">コマンドリスト</param>
/// <param name="_obj">三角形のデータ作成したObject変数</param>
/// <param name="_textureHandle">テクスチャハンドル</param>
void DrawTriangle(ID3D12GraphicsCommandList* _commandList, Object* _obj, int _textureHandle = -1);

/// <summary>
/// スプライトの描画
/// </summary>
/// <param name="_commandList">コマンドリスト</param>
/// <param name="_obj">スプライトのデータ作成したObject変数</param>
/// <param name="_textureHandle">テクスチャハンドル</param>
void DrawSprite(ID3D12GraphicsCommandList* _commandList, Object* _obj, int _textureHandle = -1);

void DrawSphere(ID3D12GraphicsCommandList* _commandList, Object* _obj, ID3D12Resource* _light, int _textureHandle = -1);

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	///COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	/// ウィンドウクラスを登録する
	WNDCLASS wc{};
	// ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウィンドウクラス名(なんでもいい)
	wc.lpszClassName = L"CGWindowClass";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// ウィンドウクラスを登録する
	RegisterClass(&wc);

	/// ウィンドウサイズを決める
	// クライアント領域のサイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	// ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	//クライアント領域をもとに実際のサイズをwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	/// ウィンドウを生成して表示
	// ウィンドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName,		// 利用するクラス名
		L"CG2",					// タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	// よく見るウィンドウスタイル
		CW_USEDEFAULT,			// 表示X座標(WindowsにOS任せる)
		CW_USEDEFAULT,			// 表示Y座標(WindowsOSに任せる)
		wrc.right - wrc.left,	// ウィンドウ横幅
		wrc.bottom - wrc.top,	// ウィンドウ立幅
		nullptr,				// 親ウィンドウハンドル
		nullptr,				// メニューハンドル
		wc.hInstance,			// インスタンスハンドル
		nullptr);				// オプション

	//ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);

	///デバッグレイヤー
#ifdef _DEBUG

	ID3D12Debug1* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		//デバッグレイヤーを有効化する
		debugController->EnableDebugLayer();
		//さらにGPU制御側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}

#endif // _DEBUG


	/// DXGIFactoeyの生成
	// DXGIファクトリーの生成
	IDXGIFactory7* dxgiFactory = nullptr;
	// HRESULTはWindows系のエラーコードであり，
	// 関数が成功したかどうかをSUCCEEDE各炉で判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違がっているか，どうにもできない場合が多いので
	// assertしておく
	assert(SUCCEEDED(hr));

	/// 使用するアダプタ(GPU)を決定する
	//使用するアダプタ用の変数。最初にnullptrを入れておく
	IDXGIAdapter4* useAdapter = nullptr;
	//良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; i++)
	{
		//アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得でき何のは一大事
		//ソフトウェアアダプタでなければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			//採用したアダプタの情報をログに出力。wstringの方なので注意。
			Log(ConvertString(std::format(L"\nUse Adapater:{}\n", adapterDesc.Description)));
			break;
		}
		//ソフトウェアアダプタの場合は見なかったことにする
		useAdapter = nullptr;
	}
	//適切なアダプターが見つからなかったので起動できない
	assert(useAdapter != nullptr);

	///D3D12Deviceの生成
	ID3D12Device* device = nullptr;
	//機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLvelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); i++)
	{
		//採用したアダプタでデバイスを生成
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		//指定したレベルでデバイスが生成で来たかを確認
		if (SUCCEEDED(hr))
		{
			//生成で来たのでログの出力を行ってループを抜ける
			Log(std::format("\nFeatureLevel : {}\n", featureLvelStrings[i]));
			break;
		}
	}
	//デバイスの生成がうまくいかなかったので起動できない
	assert(device != nullptr);
	//初期化完了のログ
	Log("Complete create D3D12Device\n");


	///エラー警告即停止
#ifdef _DEBUG

	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		//やばいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		//エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//警告時に泊まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		//解放
		infoQueue->Release();

		//抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			//Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
			//URL省略
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		//抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		//指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);

	}

#endif // _DEBUG



	/// CommandQueueを生成する
	//コマンドキューを生成する
	ID3D12CommandQueue* commandQueue = nullptr;         //commandListをGPUに投げて実行する人
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	//コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	/// commandListを生成する
	//コマンドアロケータを生成する
	ID3D12CommandAllocator* commandAllocator = nullptr; //命令保存用のメモリ管理機構 commandListとワンセット
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//コマンドアロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドリストを生成する
	ID3D12GraphicsCommandList* commandList = nullptr;           //まとまった命令群
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	/// SwapChainを生成する
	//スワップチェーンを生成する
	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;                             //画面の幅。ウィンドウクラスのクライアント領域を同じものにしておく
	swapChainDesc.Height = kClientHeight;                           //画面の高さ。ウィンドウクラスのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;              //色の形式
	swapChainDesc.SampleDesc.Count = 1;                             //マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;    //描画ターゲットとして利用すbる
	swapChainDesc.BufferCount = 2;                                  //ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;       //モニタにうつしたら中身を破棄
	//コマンドキュー，ウィンドウハンドル，設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
	assert(SUCCEEDED(hr));

	///DescriptorHeapを生成する

	//DescriptorSizeを取得しておく
	const uint32_t desriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t desriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t desriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないのでShaderVisibleはfalse
	ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);              //viewの情報を格納している場所(Discriptor)の束(配列)


	//imguiを使うためSRV用のが必要
	//SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなのでShaderVisivleはtrue
	ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	/// SwapChainからResourceを引っ張ってくる
	//SwapChainからResourceを引っ張ってくる
	ID3D12Resource* swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	/// RTVを作る
	//rtvの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;         //出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;    //2dテクスチャとして書き込む
	//ディスクリプタの先頭を取得する
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//RTVを２つ作るのでディスクリプタを２つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtVHandles[2];
	//まずは１つ目を作る。１つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある
	//rtVHandles[0] = rtvStartHandle;
	rtVHandles[0] = GetCPUDescriptorHandle(rtvDescriptorHeap, desriptorSizeRTV, 0);
	device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtVHandles[0]);
	//２つ目のディスクリプタハンドルを得る
	//rtVHandles[1].ptr = rtVHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	rtVHandles[1] = GetCPUDescriptorHandle(rtvDescriptorHeap, desriptorSizeRTV, 1);
	//２つ目を作る
	device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtVHandles[1]);


	///FenceとEventを生成
	//初期値０でFenceを作る
	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	//FenceのSignalを待つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);


	/// DXCの初期化
	// dxcCompilerを初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	//現時点でinclideしないが,includeに対応するための設定を行っておく
	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));


	/// RootSignatrueを生成する
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//descriptorRange
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;//０から始まる
	descriptorRange[0].NumDescriptors = 1;//数は１つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//ofsetを自動計算

	// RootParameter作成
	D3D12_ROOT_PARAMETER rootParameters[5] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;           // CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;        // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0;                           // レジスタ番号0を使う

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;           // CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;       // VertexShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0;                           // レジスタ番号0を使う

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//DescriptorTableで使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;			//pixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;		//tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);//tableで利用する数

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;           // CBVを使う
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;        // PixelShaderで使う
	rootParameters[3].Descriptor.ShaderRegister = 1;                           // レジスタ番号1を使う


	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;           // CBVを使う
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;        // PixelShaderで使う
	rootParameters[4].Descriptor.ShaderRegister = 2;                           // レジスタ番号1を使う

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);         // 配列の長さ

	//Samplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0-1の範囲外をリピート
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // あらかじめのMipmapを使う
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);



	//シリアライズしてバイナリする
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr))
	{
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	//バイナリをもとに生成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	/// InputLayoutの設定を行う
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	/// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	//すべての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


	/// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	//裏面(時計回り)を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//三角形を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	/// shaderをコンパイルする
	IDxcBlob* vertexShaderBlob = ComplieShader(L"Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr);

	IDxcBlob* pixelShaderBlob = ComplieShader(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr);

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//Depthの機能を有効にする
	depthStencilDesc.DepthEnable = true;
	//書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関数はLessEqeul つまり近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//DepthStencilの設定

	/// PSOを生成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature;                                                       // RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;                                                        // InputLayout
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };	    // VertexShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };       // PixelShader
	graphicsPipelineStateDesc.BlendState = blendDesc;                                                               // BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;                                                     // RasterizerState
	// 追加の DRTV の情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// どのように画面に色を打ち込むかの設定 (気にしなくて良い)
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// 実際に生成
	ID3D12PipelineState* graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	///色の変更
	ID3D12Resource* materialResource = CreateBufferResource(device, sizeof(VertexData) * 6);
	Vector4* materialData = nullptr;
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	*materialData = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

#pragma region 三角形

	///// VertexResourcesを生成する
	//ID3D12Resource* vertexResource = CreateBufferResource(device, sizeof(VertexData) * 6);
	//
	///// VertexBufferViewを作成する
	//// 頂点バッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	//// リソースの実際のアドレスから
	//vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//// 構造付きバッファのサイズは頂点3つ分のサイズ
	//vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;
	//// 1頂点あたりのサイズ
	//vertexBufferView.StrideInBytes = sizeof(VertexData);
	//
	//*materialData = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//
	////wvp用のリソースを作る。Matrix4x4 1つ分のサイズをサイズを用意する
	//ID3D12Resource* wvpResource = CreateBufferResource(device, sizeof(Matrix4x4));
	////データを書き込む
	//Matrix4x4* wvpData = nullptr;
	////書き込むためのアドレスを取得
	//wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	////単位行列を書き込んでおく
	//*wvpData =  MakeIdentity4x4();
	//
	//
	//ID3D12Resource* texVisiblity = CreateBufferResource(device, sizeof(float));
	//float* isVisible = nullptr;
	//texVisiblity->Map(0, nullptr, reinterpret_cast<void**>(&isVisible));
	//*isVisible = 0.0f;
	//
	///// Resourceにデータを書き込む
	//// 頂点バッファーフォーマットが定義されて
	//VertexData* vertexData = nullptr;
	//// 書き込むためのアクセス権限を取得
	//vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	//// 左下
	//vertexData[0].position = { -0.5f, -0.5f, 0.0f, 1.0f };
	//vertexData[0].texcoord = { 0.0f,1.0f };
	//// 上
	//vertexData[1].position = { 0.0f, 0.5f, 0.0f, 1.0f };
	//vertexData[1].texcoord = { 0.5f,0.0f };
	//// 右下
	//vertexData[2].position = { 0.5f, -0.5f, 0.0f, 1.0f };
	//vertexData[2].texcoord = { 1.0f,1.0f };
	//
	//// 左下
	//vertexData[3].position = { -0.5f, -0.5f, 0.5f, 1.0f };
	//vertexData[3].texcoord = { 0.0f,1.0f };
	//// 上
	//vertexData[4].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	//vertexData[4].texcoord = { 0.5f,0.0f };
	//// 右下
	//vertexData[5].position = { 0.5f, -0.5f,-0.5f, 1.0f };
	//vertexData[5].texcoord = { 1.0f,1.0f };

	//Object* triangle1 = new Object();
	//MakeTriangleData(device, triangle1);
	//Object* triangle2 = new Object();
	//MakeTriangleData(device, triangle2);

#pragma endregion


#pragma region 画像
	//sprite用の頂点リソ－スデータを作成
	//ID3D12Resource* vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 6);

	//頂点バッファビューを作成
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	// リソースの先頭のアドレスから使う
	//vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	// 構造付きバッファのサイズは頂点6つ分のサイズ
	//vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	// 1頂点あたりのサイズ
	//vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);


	//VertexData* vertexDataSprite = nullptr;
	////vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

	//// 1枚目の三角形
	//vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; // 左下
	//vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	//vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
	//vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
	//vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
	//vertexDataSprite[2].texcoord = { 1.0f, 1.0f };

	//// 2枚目の三角形
	//vertexDataSprite[3].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
	//vertexDataSprite[3].texcoord = { 0.0f, 0.0f };
	//vertexDataSprite[4].position = { 640.0f, 0.0f, 0.0f, 1.0f }; // 右上
	//vertexDataSprite[4].texcoord = { 1.0f, 0.0f };
	//vertexDataSprite[5].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
	//vertexDataSprite[5].texcoord = { 1.0f, 1.0f };

	//vertexDataSprite[0].normal = { 0.0f,0.0f,-1.0f };
	//vertexDataSprite[1].normal = { 0.0f,0.0f,-1.0f };
	//vertexDataSprite[2].normal = { 0.0f,0.0f,-1.0f };
	//vertexDataSprite[3].normal = { 0.0f,0.0f,-1.0f };
	//vertexDataSprite[4].normal = { 0.0f,0.0f,-1.0f };
	//vertexDataSprite[5].normal = { 0.0f,0.0f,-1.0f };

	//ID3D12Resource* texVisiblitySprite = CreateBufferResource(device, sizeof(float));
	//float* visible = nullptr;
	//texVisiblitySprite->Map(0, nullptr, reinterpret_cast<void**>(&visible));
	//*visible = 1.0f;

	//// Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	//ID3D12Resource* WvpMatrixResourceSprite = CreateBufferResource(device, sizeof(Matrix4x4));
	//// データを書き込む
	//Matrix4x4* WvpMatrixDataSprite = nullptr;
	//// 書き込むためのアドレスを取得
	//WvpMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&WvpMatrixDataSprite));
	//// 単位行列を書きこんでおく
	//*WvpMatrixDataSprite = MakeIdentity4x4();

	//ID3D12Resource* materialResorceSprite = CreateBufferResource(device, sizeof(Material));
	//Material* colorSprite;
	//materialResorceSprite->Map(0, nullptr, reinterpret_cast<void**>(&colorSprite));
	//colorSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//colorSprite->enabledLighthig = false;

	ID3D12Resource* vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 6);

	//頂点バッファビューを作成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	//リソースの先頭のアドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	// 構造付きバッファのサイズは頂点6つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	// 1頂点あたりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);


	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

	// 1枚目の三角形
	vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; // 左下
	vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
	vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
	vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
	vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
	vertexDataSprite[3].position = { 640.0f, 0.0f, 0.0f, 1.0f }; // 右上
	vertexDataSprite[3].texcoord = { 1.0f, 0.0f };

	vertexDataSprite[0].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[1].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[2].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[3].normal = { 0.0f,0.0f,-1.0f };

	ID3D12Resource* indexResourceSprite = CreateBufferResource(device, sizeof(uint32_t) * 6);

	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0;	indexDataSprite[1] = 1;	indexDataSprite[2] = 2;
	indexDataSprite[3] = 1;	indexDataSprite[4] = 3;	indexDataSprite[5] = 2;


	ID3D12Resource* texVisiblitySprite = CreateBufferResource(device, sizeof(float));
	float* visible = nullptr;
	texVisiblitySprite->Map(0, nullptr, reinterpret_cast<void**>(&visible));
	*visible = 1.0f;

	// Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	ID3D12Resource* WvpMatrixResourceSprite = CreateBufferResource(device, sizeof(TransformationMatrix));
	// データを書き込む
	TransformationMatrix* WvpMatrixDataSprite = nullptr;
	// 書き込むためのアドレスを取得
	WvpMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&WvpMatrixDataSprite));
	// 単位行列を書きこんでおく
	WvpMatrixDataSprite->World = MakeIdentity4x4();

	ID3D12Resource* materialResorceSprite = CreateBufferResource(device, sizeof(Material));
	Material* materialDataSprite;
	materialResorceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialDataSprite->enabledLighthig = false;
	materialDataSprite->uvTransform = MakeIdentity4x4();

	//Object* sprite = new Object;
	//MakeSpriteData(device, sprite);

#pragma endregion

#pragma region 球
	//
	//	//分割数
	//	const uint32_t kSubdivision = 16;
	//	const uint32_t sphereVertexNum = kSubdivision * kSubdivision * 6;
	//	//sphere用の頂点リソ－スデータを作成
	//	ID3D12Resource* vertexResourceSphere = CreateBufferResource(device, sizeof(VertexData) * sphereVertexNum);
	//
	//	//頂点バッファビューを作成
	//	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};
	//	// リソースの先頭のアドレスから使う
	//	vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();
	//	// 構造付きバッファのサイズは頂点6つ分のサイズ
	//	vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * sphereVertexNum;
	//	// 1頂点あたりのサイズ
	//	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);
	//
	//	// Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	//	ID3D12Resource* transformationMatrixResourceSphere = CreateBufferResource(device, sizeof(Matrix4x4));
	//	// データを書き込む
	//	Matrix4x4* transformationMatrixDataSphere = nullptr;
	//	// 書き込むためのアドレスを取得
	//	transformationMatrixResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSphere));
	//	// 単位行列を書きこんでおく
	//	*transformationMatrixDataSphere =  MakeIdentity4x4();
	//
	//
	//	VertexData* vertexDataSphere = nullptr;
	//	vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));
	//
	//	//vertexの計算
	//	const float kLatEvery = (float)M_PI / (float)kSubdivision;          // 緯度分割１つ分の角度 θ
	//	const float kLonEvery = (float)M_PI * 2.0 / (float)kSubdivision;    // 経度分割１つ分の角度 φ
	//
	//	//緯度の方向に分割   -π/2 ~ π/2
	//	for (uint32_t latIndex = 0; latIndex < kSubdivision; latIndex++)
	//	{
	//		float lat = -(float)M_PI / 2.0f + kLatEvery * latIndex;         // 現在の緯度
	//
	//		// 経度の方向に分割   0 ~ π
	//		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; lonIndex++)
	//		{
	//			uint32_t startIndex = (latIndex * kSubdivision + lonIndex) * 6;
	//			float lon = lonIndex * kLonEvery;                           // 現在の経度
	//
	//			//a
	//			vertexDataSphere[startIndex].position.x = std::cosf(lat) * std::cosf(lon);
	//			vertexDataSphere[startIndex].position.y = std::sinf(lat);
	//			vertexDataSphere[startIndex].position.z = std::cosf(lat) * std::sinf(lon);
	//			vertexDataSphere[startIndex].position.w = 1.0f;
	//			vertexDataSphere[startIndex].texcoord.x = float(lonIndex) / float(kSubdivision);
	//			vertexDataSphere[startIndex++].texcoord.y = 1.0f - float(latIndex) / float(kSubdivision);
	//
	//			//b
	//			vertexDataSphere[startIndex].position.x = std::cosf(lat + kLatEvery) * std::cosf(lon);
	//			vertexDataSphere[startIndex].position.y = std::sinf(lat + kLatEvery);
	//			vertexDataSphere[startIndex].position.z = std::cosf(lat + kLatEvery) * std::sinf(lon);
	//			vertexDataSphere[startIndex].position.w = 1.0f;
	//			vertexDataSphere[startIndex].texcoord.x = float(lonIndex) / float(kSubdivision);
	//			vertexDataSphere[startIndex++].texcoord.y = 1.0f - float(latIndex + 1) / float(kSubdivision);
	//
	//			//c
	//			vertexDataSphere[startIndex].position.x = std::cosf(lat) * std::cosf(lon + kLonEvery);
	//			vertexDataSphere[startIndex].position.y = std::sinf(lat);
	//			vertexDataSphere[startIndex].position.z = std::cosf(lat) * std::sinf(lon + kLonEvery);
	//			vertexDataSphere[startIndex].position.w = 1.0f;
	//			vertexDataSphere[startIndex].texcoord.x = float(lonIndex + 1) / float(kSubdivision);
	//			vertexDataSphere[startIndex++].texcoord.y = 1.0f - float(latIndex) / float(kSubdivision);
	//
	//			//bコピー
	//			vertexDataSphere[startIndex++] = vertexDataSphere[startIndex - 2];
	//
	//			//d
	//			vertexDataSphere[startIndex].position.x = std::cosf(lat + kLatEvery) * std::cosf(lon + kLonEvery);
	//			vertexDataSphere[startIndex].position.y = std::sinf(lat + kLatEvery);
	//			vertexDataSphere[startIndex].position.z = std::cosf(lat + kLatEvery) * std::sinf(lon + kLonEvery);
	//			vertexDataSphere[startIndex].position.w = 1.0f;
	//			vertexDataSphere[startIndex].texcoord.x = float(lonIndex + 1) / float(kSubdivision);
	//			vertexDataSphere[startIndex++].texcoord.y = 1.0f - float(latIndex + 1) / float(kSubdivision);
	//
	//			//cコピー
	//			vertexDataSphere[startIndex] = vertexDataSphere[startIndex - 3];
	//
	//
	//		};
	//	}
	//
	//

	//Object* sphere = new Object();
	//MakeSphereData(device, sphere);

#pragma region 平行光源
	DirectionalLight* directionalLightData = nullptr;
	ID3D12Resource* directionalLightResource = CreateBufferResource(device, sizeof(DirectionalLight));
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;
	directionalLightData->isHalf = true;

#pragma endregion
	ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, kClientWidth, kClientHeight);

	ID3D12DescriptorHeap* dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//基本的にはresourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2dTexture
	//DSVHeapの先頭にDSVを作る
	device->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


	D3D12_VIEWPORT viewport{};
	// ビューポート領域のサイズを一緒にして画面全体を表示
	viewport.Width = kClientWidth;
	viewport.Height = kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissorRect{};
	// シザー矩形
	// scissorRect.left – ビューポートと同じ幅と高さに設定されることが多い
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;

	/*DirectX::ScratchImage mipImages = LoadTexture("resources/images/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	ID3D12Resource* textureResource = CreateTextureResource(device, metadata);
	ID3D12Resource* intermediateResource = UploadTextureData(textureResource, mipImages, device, commandList);

	///テクスチャの読み込み
	DirectX::ScratchImage mipImages2 = LoadTexture("resources/images/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	ID3D12Resource* textureResource2 = CreateTextureResource(device, metadata2);
	ID3D12Resource* intermediateResource2 = UploadTextureData(textureResource2, mipImages2, device, commandList);*/

	int cubeGH = LoadTexture("resources/images/cube.jpg", device, commandList, srvDescriptorHeap, desriptorSizeSRV);
	int uvGH = LoadTexture("resources/images/uvChecker.png", device, commandList, srvDescriptorHeap, desriptorSizeSRV);
	int ballGH = LoadTexture("resources/images/monsterBall.png", device, commandList, srvDescriptorHeap, desriptorSizeSRV);

	/////shaderResourceView
	////metadataをもとに作成
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	//srvDesc.Format = metadata.format;
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	//srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);


	////metadataをもとに作成
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	//srvDesc2.Format = metadata2.format;
	//srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	//srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);


	////SRVを作成するDescriptorHeapの場所を決める
	////D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandlerCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandlerCPU = GetCPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 1);
	////D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandlerGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandlerGPU = GetGPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 1);
	////先頭はImGuiが使っているのでその次を使う
	////textureSrvHandlerCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	////textureSrvHandlerGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	////SRVの生成
	//device->CreateShaderResourceView(textureResource, &srvDesc, textureSrvHandlerCPU);

	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandlerCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 2);
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandlerGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 2);
	//device->CreateShaderResourceView(textureResource2, &srvDesc, textureSrvHandlerCPU2);


	///imguiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(
		device,
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap,
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	);

	///
	/// 変数宣言
	///

	stTransform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
	stTransform cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,-10.0f} };

	stTransform spriteTrans{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };

	Vector4 objColor1 = { 1.0f, 1.0f, 1.0f, 1.0f };
	bool ishalf = true;

	stTransform unTransformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };


	int currentTexture = 0;
	const char* textureOption[] = { "cube","uvChecker","monsterBall" };

	///
	/// メインループ
	/// 
	MSG msg{};
	// ウィンドウのｘボタンが押されるまでループ
	while (msg.message != WM_QUIT)
	{
		// Windowにメッセージが来ていたら最優先で処理させる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();


			///
			/// 更新処理ここから
			/// 


			//ImGui::ShowDemoWindow();

			ImGui::Begin("Window");
			if (ImGui::CollapsingHeader("object"))
			{
				if (ImGui::TreeNode("Sphere"))
				{
					ImGui::DragFloat4("color", &objColor1.x, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
					ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
					ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
					ImGui::Combo("texture", &currentTexture, textureOption, IM_ARRAYSIZE(textureOption));
					//triangle1->materialData->color = objColor1;
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Sprite"))
				{
					ImGui::ColorEdit3("color", &objColor1.x);
					ImGui::DragFloat3("scale", &spriteTrans.scale.x, 0.01f);
					ImGui::DragFloat3("rotate", &spriteTrans.rotate.x, 0.01f);
					ImGui::DragFloat3("translate", &spriteTrans.translate.x, 1.0f);
					materialDataSprite->color = objColor1;

					if (ImGui::TreeNode("uvTransform"))
					{
						ImGui::DragFloat2("uvTranslate", &unTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
						ImGui::DragFloat2("uvScale", &unTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
						ImGui::SliderAngle("uvRotate", &unTransformSprite.rotate.z);
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("DirectionalLight"))
				{
					ImGui::DragFloat4("color", &directionalLightData->color.x, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat3("direction", &directionalLightData->direction.x, 0.01f);
					ImGui::DragFloat("intensity", &directionalLightData->intensity, 0.01f);
					ImGui::Checkbox("isHalf", &ishalf);

					directionalLightData->isHalf = ishalf;
					directionalLightData->direction = Normalize(directionalLightData->direction);
					ImGui::TreePop();
				}

			}

			ImGui::End();



			/*
			ImGui::Begin("Triangle2");
			ImGui::DragFloat4("color", &objColor2.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat3("scale", &transformTriangle2.scale.x, 0.01f);
			ImGui::DragFloat3("rotate", &transformTriangle2.rotate.x, 0.01f);
			ImGui::DragFloat3("translate", &transformTriangle2.translate.x, 0.01f);
			ImGui::Checkbox("IsDraw", &Triangle2IsDraw);
			ImGui::Checkbox("isRotate", &autoYRotate_2);
			ImGui::End();*/


			/*ImGui::Begin("Sprite");
			ImGui::DragFloat3("scale", &transformSprite.scale.x, 0.01f);
			ImGui::DragFloat3("rotate", &transformSprite.rotate.x, 0.01f);
			ImGui::DragFloat3("translate", &transformSprite.translate.x, 1.0f);
			ImGui::End();

			ImGui::Begin("sphere");
			ImGui::DragFloat3("scale", &transformSphere.scale.x, 0.01f);
			ImGui::DragFloat3("rotate", &transformSphere.rotate.x, 0.01f);
			ImGui::DragFloat3("translate", &transformSphere.translate.x, 0.01f);
			ImGui::End();*/

			Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 viewProjectionMatrix = viewMatrix * projectionMatrix;

			/*TransformationMatrix transMat;
			transMat.World =  MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			transMat.WVP =  Multiply(transMat.World,  Multiply(viewMatrix, projectionMatrix));
			*triangle1->transformMat = transMat;*/

			//*sphere->transformMat = CalculateObjectWVPMat(transform, viewProjectionMatrix);

			/*worldMatrix =  MakeAffineMatrix(spriteTrans.scale, spriteTrans.rotate, spriteTrans.translate);
			Matrix4x4 viewMatSprite =  MakeIdentity4x4();
			projectionMatrix =  MakeOrthographicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
			worldViewProjectionMatrix =  Multiply(worldMatrix,  Multiply(viewMatSprite, projectionMatrix));
			*WvpMatrixDataSprite = worldViewProjectionMatrix;*/
			*WvpMatrixDataSprite = CalculateSpriteWVPMat(spriteTrans);

			Matrix4x4 uvTransformMatrix = MakeScaleMatrix(unTransformSprite.scale);
			uvTransformMatrix = uvTransformMatrix * MakeRotateZMatrix(unTransformSprite.rotate.z);
			uvTransformMatrix = uvTransformMatrix * MakeTranslateMatrix(unTransformSprite.translate);
			materialDataSprite->uvTransform = uvTransformMatrix;

			/*
			transformSphere.rotate.y += 0.01f;
			Matrix4x4 worldMatrix =  MakeAffineMatrix(transformSphere.scale, transformSphere.rotate, transformSphere.translate);
			Matrix4x4 cameraMatrix =  MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix =  Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix =  MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix =  Multiply(worldMatrix,  Multiply(viewMatrix, projectionMatrix));*/
			//worldMatrix =  MakeAffineMatrix(transformSphere.scale, transformSphere.rotate, transformSphere.translate);
			//worldViewProjectionMatrix =  Multiply(worldMatrix,  Multiply(viewMatrix, projectionMatrix));
			//*sphere->wvpData = worldViewProjectionMatrix;

			///
			/// 更新処理ここまで
			///

			//これから書き込むバックバッファのインデックスを取得
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

			//trasitionBarrierを貼るコード
			D3D12_RESOURCE_BARRIER barrier{};
			//今回のバリアはtransition
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			//Noneにしておく
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			//バリアを貼る対象のリソース。現在のバックバッファに対して行う
			barrier.Transition.pResource = swapChainResources[backBufferIndex];
			//遷移前（現在）のResourceState
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			//遷移後のResourceState
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			//transitionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);


			//描画先のRTVを設定する
			//commandList->OMSetRenderTargets(1, &rtVHandles[backBufferIndex], false, nullptr);
			//指定した色で画面算体をクリアする

			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);

			//描画先とRTVとDSVの設定を行う
			//D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetCPUDescriptorHandle(dsvDescriptorHeap, desriptorSizeDSV, 0);
			commandList->OMSetRenderTargets(1, &rtVHandles[backBufferIndex], false, &dsvHandle);

			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
			commandList->ClearRenderTargetView(rtVHandles[backBufferIndex], clearColor, 0, nullptr);

			//指定した深度で画面をクリアする
			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			commandList->RSSetViewports(1, &viewport);                            // Viewportを設定
			commandList->RSSetScissorRects(1, &scissorRect);                      // Scissorを設定


			///
			/// 描画ここから 
			/// 

			//TODO:draw関数を作りたい
			//(x-min)/(max-min);


			// RootSignatureを設定。PSOに設定しているものと同じ必要がある
			commandList->SetGraphicsRootSignature(rootSignature);
			commandList->SetPipelineState(graphicsPipelineState);                 // PSOを設定

			// 頂点形式を。PSOに設定しているものと同じだが別途、同じものを設定することが必要らしい。
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			//DrawSphere(commandList, sphere, directionalLightResource, ballGH);
			//DrawTriangle(commandList, triangle1, currentTexture);
			//DrawSprite(commandList, sprite, uvGH);

			/*commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);

			commandList->SetGraphicsRootConstantBufferView(0, materialResorceSprite->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, WvpMatrixResourceSprite->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(1));
			commandList->SetGraphicsRootConstantBufferView(3, texVisiblitySprite->GetGPUVirtualAddress());

			commandList->DrawInstanced(6, 1, 0, 0);*/

			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
			commandList->IASetIndexBuffer(&indexBufferViewSprite);

			commandList->SetGraphicsRootConstantBufferView(0, materialResorceSprite->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(1, WvpMatrixResourceSprite->GetGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(1));
			commandList->SetGraphicsRootConstantBufferView(3, texVisiblitySprite->GetGPUVirtualAddress());
			commandList->SetGraphicsRootConstantBufferView(4, directionalLightResource->GetGPUVirtualAddress());

			commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			//commandList->IASetVertexBuffers(0, 1, &triangle2->vertexBufferView);             // VBを設定
			//*triangle2->materialData = objColor2;

			//commandList->SetGraphicsRootConstantBufferView(0, triangle2->materialResource->GetGPUVirtualAddress());
			//commandList->SetGraphicsRootConstantBufferView(1, triangle2->wvpResource->GetGPUVirtualAddress());
			//commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandlerGPU);
			//commandList->DrawInstanced(3, 1, 0, 0);


		///
		/// 描画ここまで
		/// 

			ImGui::Render();

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

			//画面に書く処理はすべて終わり，画面に移すので状態を遷移
			//今回はRenderTargetからPresentにする
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			//TransitionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);

			//コマンドリストの内容を確立させる。すべてのコマンドを積んでからcloseすること
			hr = commandList->Close();
			assert(SUCCEEDED(hr));

			/// コマンドをキックする
			//GPUにコマンドリストの実行を行わせる
			ID3D12CommandList* commandLists[] = { commandList };
			commandQueue->ExecuteCommandLists(1, commandLists);
			//GPUとOSに画面の交換を行うように通知する
			swapChain->Present(1, 0);			//	画面が切り替わる

			/// GPUにSignalを送る
			//Fenceの値の更新
			fenceValue++;
			//GPUがここまでたどり着いたときに，Fenceの値を指定した値に代入するようにSignalを送る
			commandQueue->Signal(fence, fenceValue);

			//Fenceの値が指定したSignal値にたどり着いているか確認する
			//GetCompleteValueの初期値はFence作成時に渡した初期値
			if (fence->GetCompletedValue() < fenceValue)
			{
				//指定したSignalにたどり着いていないので，たどり着くまで待つようにイベントを設定する
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				//イベント待つ
				WaitForSingleObject(fenceEvent, INFINITE);
			}

			//次のフレーム用のコマンドリストを準備
			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));
			hr = commandList->Reset(commandAllocator, nullptr);
			assert(SUCCEEDED(hr));

		}
	}

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	//delete triangle1;
	//delete triangle2;

	dsvDescriptorHeap->Release();
	depthStencilResource->Release();
	//transformationMatrixResourceSprite->Release();
	//vertexResourceSprite->Release();
	//intermediateResource2->Release();
	//textureResource2->Release();
	//intermediateResource->Release();
	//textureResource->Release();
	//wvpResource->Release();
	materialResource->Release();
	//vertexResource->Release();
	graphicsPipelineState->Release();
	signatureBlob->Release();
	if (errorBlob)
	{
		errorBlob->Release();
	}
	rootSignature->Release();
	pixelShaderBlob->Release();
	vertexShaderBlob->Release();


	includeHandler->Release();
	dxcCompiler->Release();
	dxcUtils->Release();
	CloseHandle(fenceEvent);
	fence->Release();
	swapChainResources[0]->Release();
	swapChainResources[1]->Release();
	srvDescriptorHeap->Release();
	rtvDescriptorHeap->Release();
	swapChain->Release();
	commandList->Release();
	commandAllocator->Release();
	commandQueue->Release();
	device->Release();
	useAdapter->Release();
	dxgiFactory->Release();
#ifdef _DEBUG
	debugController->Release();
#endif // _DEBUG
	CloseWindow(hwnd);


	///ReportLiveObjects
	//リソースリークチェック
	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
	{
		//debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		//debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		//debug->Release();
	}

	CoUninitialize();

	return 0;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true;
	}

	// メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		// ウィンドウが破棄された
	case WM_DESTROY:
		// OSに対して，アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string& message)
{
	OutputDebugStringA(message.c_str());
}

std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

IDxcBlob* ComplieShader(const std::wstring& _filePath, const wchar_t* _profile, IDxcUtils* _dxcUtils, IDxcCompiler3* _dxcCompiler, IDxcIncludeHandler* _includeHandler)
{
	//hlslファイルを読み込む
	//これからシェーダーをコンパイルする旨をログに出す
	Log(ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", _filePath, _profile)));
	//hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = _dxcUtils->LoadFile(_filePath.c_str(), nullptr, &shaderSource);
	//読めなかったら止める
	assert(SUCCEEDED(hr));
	//読み込んだ内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	//Cmpileする
	LPCWSTR arguments[] = {
		_filePath.c_str(),      //コンパイル対象のhlslファイル名
		L"-E",L"main",          //エントリーポイントの指定。基本的にmain以外にはしない
		L"-T",_profile,         // shaderprofilerの設定
		L"-Zi",L"^Qembed_debug" // デバッグ用の情報を埋め込む
		L"-Od",                 // 最適化外しておく
		L"-Zpr",                // メモリレイアウトは行優先
	};
	//実際にshaderをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = _dxcCompiler->Compile(
		&shaderSourceBuffer,            // 読み込んだファイル
		arguments,			            // コンパイルオプション
		_countof(arguments),            // コンパイルオプションの数
		_includeHandler,	            // includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)     // コンパイル結果
	);

	assert(SUCCEEDED(hr));

	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		Log(shaderError->GetStringPointer());
		assert(false);
	}

	//コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	//成功したログを出す
	Log(ConvertString(std::format(L"Compile Succesed,path:{},profile:{}\n", _filePath, _profile)));
	//もう使わないリソースを解放
	shaderSource->Release();
	shaderResult->Release();

	//実行用バイナリを返却
	return shaderBlob;
}

ID3D12Resource* CreateBufferResource(ID3D12Device* _device, size_t _sizeInBytes)
{
	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;               // UploadHeapを使う

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	// バッファリソース。テクスチャの場合は別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = _sizeInBytes;                  // リソースのサイズ。今回はVector4を3個分
	//バッファの場合はこれらを１にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	//バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	//実際に頂点リソースを作る
	ID3D12Resource* vertexResource = nullptr;
	HRESULT hr = _device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

	return vertexResource;
}

ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* _device, int32_t _width, int32_t _height)
{
	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = _width; // Textureの幅
	resourceDesc.Height = _height; // Textureの高さ
	resourceDesc.MipLevels = 1; // mipmapの数
	resourceDesc.DepthOrArraySize = 1; // 奥行き or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント、通常は1
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う指定
	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

	//震度のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;//1.0f(最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//フォーマット，Resourceと合わせる

	// Resourceの生成
	ID3D12Resource* resource = nullptr;
	HRESULT hr = _device->CreateCommittedResource(
		&heapProperties, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // Heapの特別な設定は特になし。
		&resourceDesc, // Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込み状態にしておく
		&depthClearValue, // Clear値の値
		IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));

	//resource->Release();

	return resource;
}

ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* _device, D3D12_DESCRIPTOR_HEAP_TYPE _heapType, UINT _numDescriptors, bool _shaderVisible)
{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;              //viewの情報を格納している場所(Discriptor)の束(配列)
	D3D12_DESCRIPTOR_HEAP_DESC descRiptorHeapDesc{};
	descRiptorHeapDesc.Type = _heapType;    //レンダーターゲットビュー(RTV)用
	descRiptorHeapDesc.NumDescriptors = _numDescriptors;                       //ダブルバッファ用に２つ。多くもかまわない
	descRiptorHeapDesc.Flags = _shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = _device->CreateDescriptorHeap(&descRiptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	//ディスクリプターヒープが生成できなかったので起動できない
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

DirectX::ScratchImage LoadTexture(const std::string& _filePath)
{
	DirectX::ScratchImage image{};
	std::wstring filePathw = ConvertString(_filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathw.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	//ミップマップの生成
	DirectX::ScratchImage mipImage{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImage);
	assert(SUCCEEDED(hr));

	//ミップマップ付きのデータを返す	
	return mipImage;
}

ID3D12Resource* CreateTextureResource(ID3D12Device* _device, const DirectX::TexMetadata& _metadata)
{
	// metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(_metadata.width); // Textureの幅
	resourceDesc.Height = UINT(_metadata.height); // Textureの高さ
	resourceDesc.MipLevels = UINT16(_metadata.mipLevels); // mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(_metadata.arraySize); // 奥行き or 配列Textureの配列数
	resourceDesc.Format = _metadata.format; // Textureのフォーマット 
	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント、通常は1
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(_metadata.dimension); // Textureの次元。省略はしているのは2次元


	// 利用するHeapの設定。非常に特殊な運用、02_04から一部的なケース版がある
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 細かい設定を行う

	//heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
	//heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; // プロセッサの近くに配置

	//resourceを生成
	ID3D12Resource* resource = nullptr;
	HRESULT hr = _device->CreateCommittedResource(
		&heapProperties,//heapの設定
		D3D12_HEAP_FLAG_NONE,//heapの特殊な設定。特になし
		&resourceDesc,//resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST,//データ転送される設定
		nullptr,//clearの最適値 使わないのでnullptr
		IID_PPV_ARGS(&resource));//作成するresourceポインタへのポインタ
	assert(SUCCEEDED(hr));

	return resource;
}

[[nodiscard]]
ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList)
{

	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	ID3D12Resource* intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList, texture, intermediateResource, 0, 0, UINT(subresources.size()), subresources.data());
	//Tetureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* _descriptorHeap, uint32_t _descriptorSize, uint32_t _index)
{

	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = _descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (_descriptorSize * _index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* _descriptorHeap, uint32_t _descriptorSize, uint32_t _index)

{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = _descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (_descriptorSize * _index);
	return handleGPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(int _textureHandle)
{
	assert(textures.size() > _textureHandle);
	return textures[_textureHandle].srvHandlerGPU;
}

int LoadTexture(const std::string& _filePath, ID3D12Device* _device, ID3D12GraphicsCommandList* _commandList, ID3D12DescriptorHeap* _srvDescriptorHeap, uint32_t _srvSize)
{
	auto it = std::find_if(textures.begin(), textures.end(), [&](const auto& texture) {
		return texture.name == _filePath;
		});

	if (it != textures.end())
	{
		return static_cast<int>(std::distance(textures.begin(), it));
	}

	textures.push_back(Texture());

	size_t index = textures.size() - 1;
	textures[index].name = _filePath;

	DirectX::ScratchImage mipImages = LoadTexture(_filePath);
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	textures[textures.size() - 1].resource = CreateTextureResource(_device, metadata);
	ID3D12Resource* intermediateResource = UploadTextureData(textures[textures.size() - 1].resource, mipImages, _device, _commandList);


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	textures[index].srvHandlerCPU = GetCPUDescriptorHandle(_srvDescriptorHeap, _srvSize, (uint32_t)index + 1);
	textures[index].srvHandlerGPU = GetGPUDescriptorHandle(_srvDescriptorHeap, _srvSize, (uint32_t)index + 1);
	_device->CreateShaderResourceView(textures[index].resource, &srvDesc, textures[index].srvHandlerCPU);

	return (int)index;
}

void MakeTriangleData(ID3D12Device* _device, Object* _obj)
{
	/// VertexResourcesを生成する
	_obj->vertexResource = CreateBufferResource(_device, sizeof(VertexData) * 3);

	/// VertexBufferViewを作成する
	// 頂点バッファビューを作成する
	_obj->vertexBufferView = { 0 };
	// リソースの実際のアドレスから
	_obj->vertexBufferView.BufferLocation = _obj->vertexResource->GetGPUVirtualAddress();
	// 構造付きバッファのサイズは頂点3つ分のサイズ
	_obj->vertexBufferView.SizeInBytes = sizeof(VertexData) * 3;
	// 1頂点あたりのサイズ
	_obj->vertexBufferView.StrideInBytes = sizeof(VertexData);

	_obj->materialData = new Material;
	_obj->materialData->color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };


	///色の変更
	_obj->materialResource = CreateBufferResource(_device, sizeof(Vector4) * 3);
	_obj->materialData = nullptr;
	_obj->materialResource->Map(0, nullptr, reinterpret_cast<void**>(&_obj->materialData));

	//wvp用のリソースを作る。Matrix4x4 1つ分のサイズをサイズを用意する
	_obj->wvpResource = CreateBufferResource(_device, sizeof(TransformationMatrix));
	//データを書き込む
	_obj->transformMat = nullptr;
	//書き込むためのアドレスを取得
	_obj->wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&_obj->transformMat));
	//単位行列を書き込んでおく
	_obj->transformMat->World = MakeIdentity4x4();

	_obj->texVisiblity = CreateBufferResource(_device, sizeof(float));
	_obj->visible = nullptr;
	_obj->texVisiblity->Map(0, nullptr, reinterpret_cast<void**>(&_obj->visible));
	*_obj->visible = 1.0f;

	/// Resourceにデータを書き込む
	// 頂点バッファーフォーマットが定義されて

	// 書き込むためのアクセス権限を取得
	_obj->vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&_obj->vertexData));
	// 左下
	_obj->vertexData[0].position = { -0.5f, -0.5f, 0.0f, 1.0f };
	_obj->vertexData[0].texcoord = { 0.0f,1.0f };
	_obj->vertexData[0].normal.x = _obj->vertexData[0].position.x;
	_obj->vertexData[0].normal.y = _obj->vertexData[0].position.y;
	_obj->vertexData[0].normal.z = _obj->vertexData[0].position.z;
	// 上
	_obj->vertexData[1].position = { 0.0f, 0.5f, 0.0f, 1.0f };
	_obj->vertexData[1].texcoord = { 0.5f,0.0f };
	_obj->vertexData[1].normal.x = _obj->vertexData[1].position.x;
	_obj->vertexData[1].normal.y = _obj->vertexData[1].position.y;
	_obj->vertexData[1].normal.z = _obj->vertexData[1].position.z;
	// 右下
	_obj->vertexData[2].position = { 0.5f, -0.5f, 0.0f, 1.0f };
	_obj->vertexData[2].texcoord = { 1.0f,1.0f };
	_obj->vertexData[2].normal.x = _obj->vertexData[2].position.x;
	_obj->vertexData[2].normal.y = _obj->vertexData[2].position.y;
	_obj->vertexData[2].normal.z = _obj->vertexData[2].position.z;

	_obj->vertexNum = 3;

}

void MakeSphereData(ID3D12Device* _device, Object* _obj)
{
	//分割数
	const uint32_t kSubdivision = 16;
	const uint32_t sphereVertexNum = kSubdivision * kSubdivision * 6;
	//sphere用の頂点リソ－スデータを作成
	_obj->vertexResource = CreateBufferResource(_device, sizeof(VertexData) * sphereVertexNum);

	//頂点バッファビューを作成
	_obj->vertexBufferView = { 0 };
	// リソースの先頭のアドレスから使う
	_obj->vertexBufferView.BufferLocation = _obj->vertexResource->GetGPUVirtualAddress();
	// 構造付きバッファのサイズは頂点6つ分のサイズ
	_obj->vertexBufferView.SizeInBytes = sizeof(VertexData) * sphereVertexNum;
	// 1頂点あたりのサイズ
	_obj->vertexBufferView.StrideInBytes = sizeof(VertexData);

	// Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	_obj->wvpResource = CreateBufferResource(_device, sizeof(TransformationMatrix));
	// データを書き込む
	_obj->transformMat = nullptr;
	// 書き込むためのアドレスを取得
	_obj->wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&_obj->transformMat));
	// 単位行列を書きこんでおく
	_obj->transformMat->World = MakeIdentity4x4();


	_obj->texVisiblity = CreateBufferResource(_device, sizeof(float));
	_obj->visible = nullptr;
	_obj->texVisiblity->Map(0, nullptr, reinterpret_cast<void**>(&_obj->visible));
	*_obj->visible = 1.0f;

	///色の変更
	_obj->materialResource = CreateBufferResource(_device, sizeof(Material) * sphereVertexNum);
	_obj->materialData = nullptr;
	_obj->materialResource->Map(0, nullptr, reinterpret_cast<void**>(&_obj->materialData));
	_obj->materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	_obj->materialData->enabledLighthig = true;

	_obj->vertexData = nullptr;
	_obj->vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&_obj->vertexData));

	//vertexの計算
	const float kLatEvery = (float)M_PI / (float)kSubdivision;          // 緯度分割１つ分の角度 θ
	const float kLonEvery = (float)M_PI * 2.0 / (float)kSubdivision;    // 経度分割１つ分の角度 φ

	//緯度の方向に分割   -π/2 ~ π/2
	for (uint32_t latIndex = 0; latIndex < kSubdivision; latIndex++)
	{
		float lat = -(float)M_PI / 2.0f + kLatEvery * latIndex;         // 現在の緯度

		// 経度の方向に分割   0 ~ π
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; lonIndex++)
		{
			uint32_t startIndex = (latIndex * kSubdivision + lonIndex) * 6;
			float lon = lonIndex * kLonEvery;                           // 現在の経度

			//a
			_obj->vertexData[startIndex].position.x = std::cosf(lat) * std::cosf(lon);
			_obj->vertexData[startIndex].position.y = std::sinf(lat);
			_obj->vertexData[startIndex].position.z = std::cosf(lat) * std::sinf(lon);
			_obj->vertexData[startIndex].position.w = 1.0f;
			_obj->vertexData[startIndex].texcoord.x = float(lonIndex) / float(kSubdivision);
			_obj->vertexData[startIndex].texcoord.y = 1.0f - float(latIndex) / float(kSubdivision);
			_obj->vertexData[startIndex].normal.x = _obj->vertexData[startIndex].position.x;
			_obj->vertexData[startIndex].normal.y = _obj->vertexData[startIndex].position.y;
			_obj->vertexData[startIndex].normal.z = _obj->vertexData[startIndex].position.z;
			_obj->vertexData[startIndex].normal = Normalize(_obj->vertexData[startIndex++].normal);

			//b
			_obj->vertexData[startIndex].position.x = std::cosf(lat + kLatEvery) * std::cosf(lon);
			_obj->vertexData[startIndex].position.y = std::sinf(lat + kLatEvery);
			_obj->vertexData[startIndex].position.z = std::cosf(lat + kLatEvery) * std::sinf(lon);
			_obj->vertexData[startIndex].position.w = 1.0f;
			_obj->vertexData[startIndex].texcoord.x = float(lonIndex) / float(kSubdivision);
			_obj->vertexData[startIndex].texcoord.y = 1.0f - float(latIndex + 1) / float(kSubdivision);
			_obj->vertexData[startIndex].normal.x = _obj->vertexData[startIndex].position.x;
			_obj->vertexData[startIndex].normal.y = _obj->vertexData[startIndex].position.y;
			_obj->vertexData[startIndex].normal.z = _obj->vertexData[startIndex].position.z;
			_obj->vertexData[startIndex].normal = Normalize(_obj->vertexData[startIndex++].normal);

			//c
			_obj->vertexData[startIndex].position.x = std::cosf(lat) * std::cosf(lon + kLonEvery);
			_obj->vertexData[startIndex].position.y = std::sinf(lat);
			_obj->vertexData[startIndex].position.z = std::cosf(lat) * std::sinf(lon + kLonEvery);
			_obj->vertexData[startIndex].position.w = 1.0f;
			_obj->vertexData[startIndex].texcoord.x = float(lonIndex + 1) / float(kSubdivision);
			_obj->vertexData[startIndex].texcoord.y = 1.0f - float(latIndex) / float(kSubdivision);
			_obj->vertexData[startIndex].normal.x = _obj->vertexData[startIndex].position.x;
			_obj->vertexData[startIndex].normal.y = _obj->vertexData[startIndex].position.y;
			_obj->vertexData[startIndex].normal.z = _obj->vertexData[startIndex].position.z;
			_obj->vertexData[startIndex].normal = Normalize(_obj->vertexData[startIndex++].normal);

			//bコピー
			_obj->vertexData[startIndex] = _obj->vertexData[startIndex - 2];
			_obj->vertexData[startIndex].normal = Normalize(_obj->vertexData[startIndex++].normal);

			//d
			_obj->vertexData[startIndex].position.x = std::cosf(lat + kLatEvery) * std::cosf(lon + kLonEvery);
			_obj->vertexData[startIndex].position.y = std::sinf(lat + kLatEvery);
			_obj->vertexData[startIndex].position.z = std::cosf(lat + kLatEvery) * std::sinf(lon + kLonEvery);
			_obj->vertexData[startIndex].position.w = 1.0f;
			_obj->vertexData[startIndex].texcoord.x = float(lonIndex + 1) / float(kSubdivision);
			_obj->vertexData[startIndex].texcoord.y = 1.0f - float(latIndex + 1) / float(kSubdivision);
			_obj->vertexData[startIndex].normal.x = _obj->vertexData[startIndex].position.x;
			_obj->vertexData[startIndex].normal.y = _obj->vertexData[startIndex].position.y;
			_obj->vertexData[startIndex].normal.z = _obj->vertexData[startIndex].position.z;
			_obj->vertexData[startIndex].normal = Normalize(_obj->vertexData[startIndex++].normal);

			//cコピー
			_obj->vertexData[startIndex] = _obj->vertexData[startIndex - 3];
			_obj->vertexData[startIndex].normal = Normalize(_obj->vertexData[startIndex].normal);
		};
	}
	_obj->vertexNum = sphereVertexNum;

}

void MakeSpriteData(ID3D12Device* _device, Object* _obj)
{
	//sprite用の頂点リソ－スデータを作成
	_obj->vertexResource = CreateBufferResource(_device, sizeof(VertexData) * 6);

	// リソースの先頭のアドレスから使う
	_obj->vertexBufferView.BufferLocation = _obj->vertexResource->GetGPUVirtualAddress();
	// 構造付きバッファのサイズは頂点6つ分のサイズ
	_obj->vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;
	// 1頂点あたりのサイズ
	_obj->vertexBufferView.StrideInBytes = sizeof(VertexData);

	_obj->vertexData = nullptr;
	_obj->vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&_obj->vertexData));

	// 1枚目の三角形
	_obj->vertexData[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; // 左下
	_obj->vertexData[0].texcoord = { 0.0f, 1.0f };
	_obj->vertexData[1].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
	_obj->vertexData[1].texcoord = { 0.0f, 0.0f };
	_obj->vertexData[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
	_obj->vertexData[2].texcoord = { 1.0f, 1.0f };
	_obj->vertexData[3].position = { 640.0f, 0.0f, 0.0f, 1.0f }; // 右上
	_obj->vertexData[3].texcoord = { 1.0f, 0.0f };

	_obj->vertexData[0].normal = { 0.0f,0.0f,-1.0f };
	_obj->vertexData[1].normal = { 0.0f,0.0f,-1.0f };
	_obj->vertexData[2].normal = { 0.0f,0.0f,-1.0f };
	_obj->vertexData[3].normal = { 0.0f,0.0f,-1.0f };

	_obj->materialResource = CreateBufferResource(_device, sizeof(Material));

	_obj->materialData = new Material;
	_obj->materialResource->Map(0, nullptr, reinterpret_cast<void**>(&_obj->materialData));
	_obj->materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	_obj->materialData->enabledLighthig = true;


	_obj->texVisiblity = CreateBufferResource(_device, sizeof(float));
	_obj->visible = nullptr;
	_obj->texVisiblity->Map(0, nullptr, reinterpret_cast<void**>(&_obj->visible));
	*_obj->visible = 1.0f;

	// Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	_obj->wvpResource = CreateBufferResource(_device, sizeof(TransformationMatrix));
	// データを書き込む
	_obj->transformMat = nullptr;
	// 書き込むためのアドレスを取得
	_obj->wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&_obj->transformMat));
	// 単位行列を書きこんでおく
	_obj->transformMat->World = MakeIdentity4x4();
	_obj->transformMat->WVP = MakeIdentity4x4();
}

TransformationMatrix CalculateSpriteWVPMat(const stTransform& _transform)
{
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);

	TransformationMatrix transMat;
	transMat.World = MakeAffineMatrix(_transform.scale, _transform.rotate, _transform.translate);
	transMat.WVP = Multiply(transMat.World, Multiply(viewMatrix, projectionMatrix));

	return TransformationMatrix(transMat);
}

TransformationMatrix CalculateObjectWVPMat(const stTransform& _transform, const Matrix4x4& _VPmat)
{
	TransformationMatrix transMat;
	transMat.World = MakeAffineMatrix(_transform.scale, _transform.rotate, _transform.translate);
	transMat.WVP = transMat.World * _VPmat;
	return TransformationMatrix(transMat);
}

void DrawTriangle(ID3D12GraphicsCommandList* _commandList, Object* _obj, int _textureHandle)
{
	_commandList->IASetVertexBuffers(0, 1, &_obj->vertexBufferView);

	_commandList->SetGraphicsRootConstantBufferView(0, _obj->materialResource->GetGPUVirtualAddress());
	_commandList->SetGraphicsRootConstantBufferView(1, _obj->wvpResource->GetGPUVirtualAddress());
	if (_textureHandle == -1)
		_commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(0));
	else
		_commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(_textureHandle));
	_commandList->SetGraphicsRootConstantBufferView(3, _obj->texVisiblity->GetGPUVirtualAddress());

	_commandList->DrawInstanced(3, 1, 0, 0);
}

void DrawSprite(ID3D12GraphicsCommandList* _commandList, Object* _obj, int _textureHandle)
{
	_commandList->IASetVertexBuffers(0, 1, &_obj->vertexBufferView);

	_commandList->SetGraphicsRootConstantBufferView(0, _obj->materialResource->GetGPUVirtualAddress());
	_commandList->SetGraphicsRootConstantBufferView(1, _obj->wvpResource->GetGPUVirtualAddress());
	if (_textureHandle == -1)
		_commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(0));
	else
		_commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(_textureHandle));
	_commandList->SetGraphicsRootConstantBufferView(3, _obj->texVisiblity->GetGPUVirtualAddress());

	_commandList->DrawInstanced(6, 1, 0, 0);
}

void DrawSphere(ID3D12GraphicsCommandList* _commandList, Object* _obj, ID3D12Resource* _light, int _textureHandle)
{
	_commandList->IASetVertexBuffers(0, 1, &_obj->vertexBufferView);

	_commandList->SetGraphicsRootConstantBufferView(0, _obj->materialResource->GetGPUVirtualAddress());
	_commandList->SetGraphicsRootConstantBufferView(1, _obj->wvpResource->GetGPUVirtualAddress());
	if (_textureHandle == -1)
		_commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(0));
	else
		_commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(_textureHandle));
	_commandList->SetGraphicsRootConstantBufferView(3, _obj->texVisiblity->GetGPUVirtualAddress());
	_commandList->SetGraphicsRootConstantBufferView(4, _light->GetGPUVirtualAddress());

	_commandList->DrawInstanced(_obj->vertexNum, 1, 0, 0);
}
