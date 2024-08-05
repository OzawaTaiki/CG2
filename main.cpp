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
#pragma comment(lib,"dxcompiler.lib")

#include <dxcapi.h>

#include "myLib/MyLib.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

#include <vector>
#include <fstream>
#include <sstream>
#include <random>
#include <numbers>
#include <map>

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _msg, WPARAM _wparam, LPARAM _lparam);

void Log(const std::string& message);
std::wstring ConvertString(const std::string& _str);
std::string ConvertString(const std::wstring& _str);


// クライアント領域のサイズ
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

const float kDeltaTime = 1.0f / 60.0f;

uint32_t globalSrvIndex = 0;

uint32_t AllocateDescriptor() {
	return globalSrvIndex++;
}


Microsoft::WRL::ComPtr<IDxcBlob> ComplieShader(
	//Complierするshaderファイルへのパス
	const std::wstring& _filePath,
	//Compilerに使用するprofile
	const wchar_t* _profile,
	//初期化で生成したものを3つ
	Microsoft::WRL::ComPtr<IDxcUtils>& _dxcUtils,
	Microsoft::WRL::ComPtr<IDxcCompiler3>& _dxcCompiler,
	Microsoft::WRL::ComPtr<IDxcIncludeHandler>& _includeHandler);

Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device>& _device, size_t _sizeInBytes);

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& _device, int32_t _width, int32_t _height);

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device>& _device, D3D12_DESCRIPTOR_HEAP_TYPE _heapType, UINT _numDescriptors, bool _shaderVisible);

//textureデータを読む
DirectX::ScratchImage LoadTexture(const std::string& _filePath);

//DirectX13のtextureリソースを作る
Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& _device, const DirectX::TexMetadata& _metadata);

//データを転送するUploadTextureData関数を作る
Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& _texture, const DirectX::ScratchImage& _mipImages, const Microsoft::WRL::ComPtr<ID3D12Device>& _device, const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList);


D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _descriptorHeap, uint32_t _descriptorSize, uint32_t _index);
D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _descriptorHeap, uint32_t _descriptorSize, uint32_t _index);



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
	float shininess;
	float useTexture;
};

struct DirectionalLight
{
	Vector4 color;		//ライトの色
	Vector3 direction;	//ライトの向き
	float intensity;	//輝度
	uint32_t isHalf;
};

struct PointLight
{
	Vector4 color;
	Vector3 position;
	float intensity;
};

struct CameraForGPU
{
	Vector3 worldPosition;
};

struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 World;
	Matrix4x4 worldInverseTranspose;
};

struct Particle
{
	stTransform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

struct Emitter
{
	stTransform transform;  //
	uint32_t count;			//発生数
	float frequency;		//発生頻度
	float frequencyTime;	//頻度用時刻
};


struct ParticleForGPU
{
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
};

struct AccelerationField
{
	Vector3 acceleration;
	AABB area;
};


struct MeshData
{
	VertexData* vertexData;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	uint32_t vertexNum;

	uint32_t* indexData;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	uint32_t indexNum;

	std::string useMaterialName;
};

struct MaterialData
{
	Material* materialData;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
};

struct TransformationData
{
	TransformationMatrix* transformMat;
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
};


struct  Object
{
	MeshData mesh;
	MaterialData material;
	TransformationData transform;

	uint32_t textureHandle;
};

struct  MultiObject
{
	std::map< std::string, MeshData> mesh;
	std::map< std::string, MaterialData> material;
	TransformationData transform;

	std::map< std::string, uint32_t>textureHandle;
};

struct Texture
{
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandlerCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandlerGPU;
	std::string name;
};
std::vector<Texture> textures;
void DeleteTextures();

// <summary>	
/// 読み込んだテクスチャを取り出す	
/// </summary>	
/// <param name="_textureHandle">テクスチャハンドル</param>	
/// <returns>テクスチャデータ</returns>	
D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(uint32_t _textureHandle);
/// <summary>	
/// テクスチャを読み込む	
/// </summary>	
/// <param name="_filePath">ファイルパス</param>	
/// <param name="_device">デバイス</param>	
/// <param name="_commandList">コマンドリスト</param>	
/// <param name="_srvDescriptorHeap">ｓｒｖディスクリプタヒープ</param>	
/// <param name="_srvSize">srvのサイズ</param>	
/// <returns>テクスチャの登録番号</returns>	
uint32_t LoadTexture(const std::string& _filePath, const Microsoft::WRL::ComPtr<ID3D12Device>& _device, const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _srvDescriptorHeap, uint32_t _srvSize);


Object LoadObjFile(const std::string& _directoryPath, const std::string& _filename, const Microsoft::WRL::ComPtr<ID3D12Device>& _device, const  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _srvDescriptorHeap, uint32_t _srvSize);

MultiObject LoadMultiObjFile(const std::string& _directoryPath, const std::string& _filename, const Microsoft::WRL::ComPtr<ID3D12Device>& _device, const  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _srvDescriptorHeap, uint32_t _srvSize);

Object findObjGroup(const MultiObject& _obj, std::string _key);

/// <summary>
/// 三角形のデータ作成
/// </summary>
/// <param name="_device">デバイス</param>
/// <param name="_obj">データ格納用Object変数</param>
Object  MakeTriangleData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device);

/// <summary>
/// 球のデータ作成
/// </summary>
/// <param name="_device">デバイス</param>
/// <param name="_obj">データ格納用Object変数</param>
Object MakeSphereData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device);

/// <summary>
/// スプライトのデータ作成
/// </summary>
/// <param name="_device">デバイス</param>
/// <param name="_obj">データ格納用Object変数</param>
Object MakeSpriteData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device);

MeshData InitializeMeshData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device, const std::vector<VertexData>& _vertex);

MaterialData InitilizeMaterialData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device);

TransformationData InitializeTransformationData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device);

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
void DrawTriangle(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const Object& _obj);

/// <summary>
/// スプライトの描画
/// </summary>
/// <param name="_commandList">コマンドリスト</param>
/// <param name="_obj">スプライトのデータ作成したObject変数</param>
/// <param name="_textureHandle">テクスチャハンドル</param>
void DrawSprite(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const Object& _obj);

void DrawSphere(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const Object& _obj);

void DrawObj(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const Object& _obj);

void DrawMultiObj(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const MultiObject& _obj);

Particle MakeNewParticle(std::mt19937& _randomEngine, const Emitter& _emitter);

std::list<Particle> Emit(const Emitter& _emitter, std::mt19937& _randomEngine);

enum class BlendMode
{
	kBlendModeNormal,
	kBlendModeAdd,
	kBlendModeSub,
	kBlendModeMultiply,
	kBlendModeScreen

};

void SetBlendMode(BlendMode _blendMode, D3D12_GRAPHICS_PIPELINE_STATE_DESC& _graphicsPipelineStateDesc);

TransformationMatrix CalculateParticleWVPMat(const stTransform& _transform, const Matrix4x4& _cameraMatrix, const Matrix4x4& _VPmat, bool useBillborad);

struct D3DResourceLeakChecker
{
	~D3DResourceLeakChecker()
	{
		//リソースリークチェック
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	D3DResourceLeakChecker leakcheker;

	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	///COMの初期化	CoInitializeEx(0, COINIT_MULTITHREADED);

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

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
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
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;
	// HRESULTはWindows系のエラーコードであり，
	// 関数が成功したかどうかをSUCCEEDE各炉で判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違がっているか，どうにもできない場合が多いので
	// assertしておく
	assert(SUCCEEDED(hr));


	/// 使用するアダプタ(GPU)を決定する
	//使用するアダプタ用の変数。最初にnullptrを入れておく
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
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
	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;

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
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));
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

	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{

		//やばいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		//エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//警告時に泊まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

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
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;         //commandListをGPUに投げて実行する人
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	//コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	/// commandListを生成する
	//コマンドアロケータを生成する
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr; //命令保存用のメモリ管理機構 commandListとワンセット
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//コマンドアロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドリストを生成する
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;           //まとまった命令群
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	/// SwapChainを生成する
	//スワップチェーンを生成する
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;                             //画面の幅。ウィンドウクラスのクライアント領域を同じものにしておく
	swapChainDesc.Height = kClientHeight;                           //画面の高さ。ウィンドウクラスのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;              //色の形式
	swapChainDesc.SampleDesc.Count = 1;                             //マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;    //描画ターゲットとして利用すbる
	swapChainDesc.BufferCount = 2;                                  //ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;       //モニタにうつしたら中身を破棄
	//コマンドキュー，ウィンドウハンドル，設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));


	///DescriptorHeapを生成する

	//DescriptorSizeを取得しておく
	const uint32_t desriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t desriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t desriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないのでShaderVisibleはfalse
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);              //viewの情報を格納している場所(Discriptor)の束(配列)

	//imguiを使うためSRV用のが必要
	//SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなのでShaderVisivleはtrue
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);


	/// SwapChainからResourceを引っ張ってくる
	//SwapChainからResourceを引っ張ってくる
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2];
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));


	/// RTVを作る
	//rtvの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;         //出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;    //2dテクスチャとして書き込む

	D3D12_CPU_DESCRIPTOR_HANDLE rtVHandles[2];
	rtVHandles[0] = GetCPUDescriptorHandle(rtvDescriptorHeap, desriptorSizeRTV, 0);
	device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtVHandles[0]);

	rtVHandles[1] = GetCPUDescriptorHandle(rtvDescriptorHeap, desriptorSizeRTV, 1);
	device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtVHandles[1]);


	///FenceとEventを生成
	//初期値０でFenceを作る
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));


	//FenceのSignalを待つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);


	/// DXCの初期化
	// dxcCompilerを初期化
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	//現時点でinclideしないが,includeに対応するための設定を行っておく
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));




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

	//descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);
	//descriptionRootSignature.pStaticSamplers = staticSamplers;


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

	//マテリアル
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;           // CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;        // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0;                           // レジスタ番号0を使う

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;           // CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;       // VertexShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0;                           // レジスタ番号0を使う

	//テクスチャ
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//DescriptorTableで使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;			//pixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;		//tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);//tableで利用する数

	//directinalLight
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;

	//カメラ
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[4].Descriptor.ShaderRegister = 2;


	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);         // 配列の長さ

	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	//シリアライズしてバイナリする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr))
	{
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	//バイナリをもとに生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature = nullptr;
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
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	/// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	//裏面(時計回り)を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//三角形を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	/// shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ComplieShader(L"Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob>pixelShaderBlob = ComplieShader(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr);



	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//Depthの機能を有効にする
	depthStencilDesc.DepthEnable = true;
	//書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関数はLessEqeul つまり近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;



	/// PSOを生成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();                                                 // RootSignature
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
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

#pragma region ぱーちくる

	//D3D12_ROOT_SIGNATURE_DESC descriptionRootSignatureForInstancing{};
	//descriptionRootSignatureForInstancing.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
	//descriptorRangeForInstancing[0].BaseShaderRegister = 0;//0から始まる
	//descriptorRangeForInstancing[0].NumDescriptors = 1;//数は1つ
	//descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVを使う
	//descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//// RootParameter作成
	//D3D12_ROOT_PARAMETER rootParametersForInstancing[3] = {};
	//rootParametersForInstancing[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;           // CBVを使う
	//rootParametersForInstancing[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;        // PixelShaderで使う
	//rootParametersForInstancing[0].Descriptor.ShaderRegister = 0;                           // レジスタ番号0を使う

	//rootParametersForInstancing[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;// CBVを使う
	//rootParametersForInstancing[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;       // VertexShaderで使う
	//rootParametersForInstancing[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
	//rootParametersForInstancing[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);

	//rootParametersForInstancing[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//DescriptorTableで使う
	//rootParametersForInstancing[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;			//pixelShaderで使う
	//rootParametersForInstancing[2].DescriptorTable.pDescriptorRanges = descriptorRange;		//tableの中身の配列を指定
	//rootParametersForInstancing[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);//tableで利用する数


	//descriptionRootSignatureForInstancing.pParameters = rootParametersForInstancing;
	//descriptionRootSignatureForInstancing.NumParameters = _countof(rootParametersForInstancing);         // 配列の長さ

	//descriptionRootSignatureForInstancing.pStaticSamplers = staticSamplers;
	//descriptionRootSignatureForInstancing.NumStaticSamplers = _countof(staticSamplers);

	//Microsoft::WRL::ComPtr<ID3DBlob> signatureBlobForInstancing = nullptr;
	//Microsoft::WRL::ComPtr<ID3DBlob> errorBlobForInstancing = nullptr;
	//hr = D3D12SerializeRootSignature(&descriptionRootSignatureForInstancing, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlobForInstancing, &errorBlobForInstancing);
	//if (FAILED(hr))
	//{
	//	Log(reinterpret_cast<char*>(signatureBlobForInstancing->GetBufferPointer()));
	//	assert(false);
	//}
	////バイナリをもとに生成
	//Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignatureForInstancing = nullptr;
	//hr = device->CreateRootSignature(0, signatureBlobForInstancing->GetBufferPointer(), signatureBlobForInstancing->GetBufferSize(), IID_PPV_ARGS(&rootSignatureForInstancing));
	//assert(SUCCEEDED(hr));

	//Microsoft::WRL::ComPtr<IDxcBlob> particleVSBlob = ComplieShader(L"Particle.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	//assert(vertexShaderBlob != nullptr);

	//Microsoft::WRL::ComPtr<IDxcBlob> particlePSBlob = ComplieShader(L"Particle.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	//assert(pixelShaderBlob != nullptr);
	//D3D12_DEPTH_STENCIL_DESC depthStencilDescForInstancing{};
	//depthStencilDescForInstancing.DepthEnable = true;
	//depthStencilDescForInstancing.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	//depthStencilDescForInstancing.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDescForInstancing{};
	//graphicsPipelineStateDescForInstancing.pRootSignature = rootSignatureForInstancing.Get();                                                 // RootSignature
	//graphicsPipelineStateDescForInstancing.InputLayout = inputLayoutDesc;                                                        // InputLayout
	//graphicsPipelineStateDescForInstancing.VS = { particleVSBlob->GetBufferPointer(), particleVSBlob->GetBufferSize() };	        // VertexShader
	//graphicsPipelineStateDescForInstancing.PS = { particlePSBlob->GetBufferPointer(), particlePSBlob->GetBufferSize() };         // PixelShader
	////graphicsPipelineStateDescForInstancing.BlendState = blendDesc;                                                               // BlendState
	//SetBlendMode(BlendMode::kBlendModeNormal, graphicsPipelineStateDescForInstancing);
	//graphicsPipelineStateDescForInstancing.RasterizerState = rasterizerDesc;                                                     // RasterizerState

	//graphicsPipelineStateDescForInstancing.NumRenderTargets = 1;
	//graphicsPipelineStateDescForInstancing.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//graphicsPipelineStateDescForInstancing.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//graphicsPipelineStateDescForInstancing.DepthStencilState = depthStencilDescForInstancing;
	//graphicsPipelineStateDescForInstancing.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//// どのように画面に色を打ち込むかの設定 (気にしなくて良い)
	//graphicsPipelineStateDescForInstancing.SampleDesc.Count = 1;
	//graphicsPipelineStateDescForInstancing.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//// 実際に生成
	//Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStateForInstancing = nullptr;
	//hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDescForInstancing, IID_PPV_ARGS(&graphicsPipelineStateForInstancing));
	//assert(SUCCEEDED(hr));
#pragma endregion

#pragma region unUV

	D3D12_ROOT_PARAMETER rootParametersForunUV[4] = {};
	//マテリアル
	rootParametersForunUV[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;           // CBVを使う
	rootParametersForunUV[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;        // PixelShaderで使う
	rootParametersForunUV[0].Descriptor.ShaderRegister = 0;                           // レジスタ番号0を使う

	rootParametersForunUV[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;           // CBVを使う
	rootParametersForunUV[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;       // VertexShaderで使う
	rootParametersForunUV[1].Descriptor.ShaderRegister = 0;                           // レジスタ番号0を使う

	//directinalLight
	rootParametersForunUV[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParametersForunUV[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParametersForunUV[2].Descriptor.ShaderRegister = 1;

	//カメラ
	rootParametersForunUV[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParametersForunUV[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParametersForunUV[3].Descriptor.ShaderRegister = 2;


	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignatureForunUV{};
	descriptionRootSignatureForunUV.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	descriptionRootSignatureForunUV.pParameters = rootParametersForunUV;
	descriptionRootSignatureForunUV.NumParameters = _countof(rootParametersForunUV);         // 配列の長さ

	descriptionRootSignatureForunUV.pStaticSamplers = staticSamplers;
	descriptionRootSignatureForunUV.NumStaticSamplers = _countof(staticSamplers);

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlobForunUV = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlobForunUV = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignatureForunUV, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlobForunUV, &errorBlobForunUV);
	if (FAILED(hr))
	{
		Log(reinterpret_cast<char*>(signatureBlobForunUV->GetBufferPointer()));
		assert(false);
	}
	//バイナリをもとに生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignatureForunUV = nullptr;
	hr = device->CreateRootSignature(0, signatureBlobForunUV->GetBufferPointer(), signatureBlobForunUV->GetBufferSize(), IID_PPV_ARGS(&rootSignatureForunUV));
	assert(SUCCEEDED(hr));



	D3D12_INPUT_ELEMENT_DESC unUVInputElementDescs[2] = {};
	unUVInputElementDescs[0].SemanticName = "POSITION";
	unUVInputElementDescs[0].SemanticIndex = 0;
	unUVInputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	unUVInputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	unUVInputElementDescs[1].SemanticName = "NORMAL";
	unUVInputElementDescs[1].SemanticIndex = 0;
	unUVInputElementDescs[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	unUVInputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC unVUInputLayoutDesc{};
	unVUInputLayoutDesc.pInputElementDescs = unUVInputElementDescs;
	unVUInputLayoutDesc.NumElements = _countof(unUVInputElementDescs);


	Microsoft::WRL::ComPtr<IDxcBlob> unUVVSBlob = ComplieShader(L"unUV.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> unUVPSBlob = ComplieShader(L"unUV.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	assert(pixelShaderBlob != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDescForunUV{};
	graphicsPipelineStateDescForunUV.pRootSignature = rootSignatureForunUV.Get();														// RootSignature
	graphicsPipelineStateDescForunUV.InputLayout = unVUInputLayoutDesc;															// InputLayout
	graphicsPipelineStateDescForunUV.VS = { unUVVSBlob->GetBufferPointer(), unUVVSBlob->GetBufferSize() };						// VertexShader
	graphicsPipelineStateDescForunUV.PS = { unUVPSBlob->GetBufferPointer(), unUVPSBlob->GetBufferSize() };						// PixelShader
	graphicsPipelineStateDescForunUV.BlendState = blendDesc;																	// BlendState
	graphicsPipelineStateDescForunUV.RasterizerState = rasterizerDesc;															// RasterizerState
	// 追加の DRTV の情報
	graphicsPipelineStateDescForunUV.NumRenderTargets = 1;
	graphicsPipelineStateDescForunUV.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDescForunUV.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	graphicsPipelineStateDescForunUV.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDescForunUV.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// どのように画面に色を打ち込むかの設定 (気にしなくて良い)
	graphicsPipelineStateDescForunUV.SampleDesc.Count = 1;
	graphicsPipelineStateDescForunUV.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStateForunUV = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDescForunUV, IID_PPV_ARGS(&graphicsPipelineStateForunUV));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region 平行光源
	DirectionalLight* directionalLightData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource = CreateBufferResource(device, sizeof(DirectionalLight));
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;
	directionalLightData->isHalf = true;

#pragma endregion

#pragma region 点光源
	/*PointLight* pointLightData = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource>pointLightResource = CreateBufferResource(device, sizeof(PointLight));
	pointLightResource->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData));

	pointLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	pointLightData->position = { 0.0f,-1.0f,0.0f };
	pointLightData->intensity = 1.0f;*/
#pragma endregion


	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = CreateDepthStencilTextureResource(device, kClientWidth, kClientHeight);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//基本的にはresourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2dTexture
	//DSVHeapの先頭にDSVを作る
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());




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




	///imguiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(
		device.Get(),
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	);


	///******************************************



	///カメラ
	CameraForGPU* cameraForGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource = CreateBufferResource(device, sizeof(CameraForGPU));
	cameraResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraForGPU));

	///
	/// 変数宣言
	///

	stTransform cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,-10.0f} };

	bool ishalf = true;

	stTransform transformObj{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
	stTransform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
	stTransform spriteTrans{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
	stTransform spriteUVTrans{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };



	int currentBlendMode = static_cast<int> (BlendMode::kBlendModeNormal);
	const char* blendModeOption[] = { "normal","add","sub","multi","screen" };

	bool enableLightting[3] = { true,true ,true };
	bool useTexture[3] = { true ,true,true };

	uint32_t uvGH = LoadTexture("resources/images/uvChecker.png", device, commandList, srvDescriptorHeap, desriptorSizeSRV);
	uint32_t cubeGH = LoadTexture("resources/images/cube.jpg", device, commandList, srvDescriptorHeap, desriptorSizeSRV);
	uint32_t ballGH = LoadTexture("resources/images/monsterBall.png", device, commandList, srvDescriptorHeap, desriptorSizeSRV);


	Object plane;
	plane = LoadObjFile("resources/obj", "plane.obj", device, commandList, srvDescriptorHeap, desriptorSizeSRV);

	stTransform planeTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };

	Object sphere;
	sphere = MakeSphereData(device);

	Object sprite;
	sprite = MakeSpriteData(device);

	Object teapot;
	teapot = LoadObjFile("resources/obj", "teapot.obj", device, commandList, srvDescriptorHeap, desriptorSizeSRV);
	stTransform teapotTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };


	Object bunny;
	bunny = LoadObjFile("resources/obj", "bunny.obj", device, commandList, srvDescriptorHeap, desriptorSizeSRV);
	stTransform bunnyTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };

	Object suzanne;
	suzanne = LoadObjFile("resources/obj", "suzanne.obj", device, commandList, srvDescriptorHeap, desriptorSizeSRV);
	stTransform suzanneTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };


	MultiObject multiMesh;
	multiMesh = LoadMultiObjFile("resources/obj", "multiMesh.obj", device, commandList, srvDescriptorHeap, desriptorSizeSRV);
	stTransform multiMeshTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };


	MultiObject multiMaterial;
	multiMaterial = LoadMultiObjFile("resources/obj", "multiMaterial.obj", device, commandList, srvDescriptorHeap, desriptorSizeSRV);
	stTransform multiMaterialTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
	stTransform multiMaterialUVTrans1{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };
	stTransform multiMaterialUVTrans2{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };

	int scene = 0;
	const char* sceneName[] = { "obj & sprite","sphere","obj & obj","suzanne","multiMesh","multiMaterial" };


	int currentTexture = 0;

	std::vector<const char*> textureOption;

	for (size_t i = 0; i < textures.size(); i++)
	{
		textureOption.push_back(textures[i].name.c_str());
	}


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
			ImGui::Combo("scene", &scene, sceneName, IM_ARRAYSIZE(sceneName));

			ImGui::BeginTabBar("camera");
			if (ImGui::BeginTabItem("Camera"))
			{
				ImGui::DragFloat3("scale", &cameraTransform.scale.x, 0.01f);
				ImGui::DragFloat3("rotate", &cameraTransform.rotate.x, 0.01f);
				ImGui::DragFloat3("translate", &cameraTransform.translate.x, 0.01f);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
			ImGui::Spacing();
			ImGui::BeginTabBar("obj");

			switch (scene)
			{
			case 0:

				if (ImGui::BeginTabItem("plane"))
				{
					ImGui::ColorEdit4("color", &plane.material.materialData->color.x);
					ImGui::DragFloat3("scale", &planeTransform.scale.x, 0.01f);
					ImGui::DragFloat3("rotate", &planeTransform.rotate.x, 0.01f);
					ImGui::DragFloat3("translate", &planeTransform.translate.x, 0.01f);
					//ImGui::Checkbox("Lighting", &enableLightting[0]);
					ImGui::Checkbox("useTexture", &useTexture[0]);
					currentTexture = static_cast<int>(plane.textureHandle);
					if (ImGui::Combo("texture", &currentTexture, textureOption.data(), static_cast<int>(textureOption.size())))
					{
						plane.textureHandle = static_cast<uint32_t> (currentTexture);
					}
					plane.material.materialData->enabledLighthig = enableLightting[0];
					plane.material.materialData->useTexture = useTexture[0] ? 1.0f : 0.0f;
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Sprite"))
				{
					ImGui::ColorEdit4("color", &sprite.material.materialData->color.x);
					ImGui::DragFloat3("scale", &spriteTrans.scale.x, 0.01f);
					ImGui::DragFloat3("rotate", &spriteTrans.rotate.x, 0.01f);
					ImGui::DragFloat3("translate", &spriteTrans.translate.x, 1.0f);
					currentTexture = static_cast<int>(sprite.textureHandle);
					if (ImGui::Combo("texture", &currentTexture, textureOption.data(), static_cast<int>(textureOption.size())))
					{
						sprite.textureHandle = static_cast<uint32_t> (currentTexture);
					}
					ImGui::Spacing();
					if (ImGui::CollapsingHeader("uvTransform"))
					{
						ImGui::DragFloat2("uvTranslate", &spriteUVTrans.translate.x, 0.01f, -10.0f, 10.0f);
						ImGui::DragFloat2("uvScale", &spriteUVTrans.scale.x, 0.01f, -10.0f, 10.0f);
						ImGui::SliderAngle("uvRotate", &spriteUVTrans.rotate.z);
						sprite.material.materialData->uvTransform = MakeAffineMatrix(spriteUVTrans.scale, spriteUVTrans.rotate, spriteUVTrans.translate);
					}
					ImGui::EndTabItem();
				}
				break;

			case 1:
				if (ImGui::BeginTabItem("Sphere"))
				{
					ImGui::ColorEdit4("color", &sphere.material.materialData->color.x);
					ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
					ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
					ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
					ImGui::Checkbox("Lighting", &enableLightting[0]);
					ImGui::Checkbox("useTexture", &useTexture[0]);
					currentTexture = static_cast<int>(sphere.textureHandle);
					if (ImGui::Combo("texture", &currentTexture, textureOption.data(), static_cast<int>(textureOption.size())))
					{
						sphere.textureHandle = static_cast<uint32_t> (currentTexture);
					}
					sphere.material.materialData->enabledLighthig = enableLightting[0];
					sphere.material.materialData->useTexture = useTexture[0] ? 1.0f : 0.0f;
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("DirectionalLight"))
				{
					ImGui::ColorEdit3("color", &directionalLightData->color.x);
					ImGui::DragFloat3("direction", &directionalLightData->direction.x, 0.01f);
					ImGui::DragFloat("intensity", &directionalLightData->intensity, 0.01f);
					ImGui::Checkbox("enable Half Lambert", &ishalf);

					directionalLightData->isHalf = ishalf;
					directionalLightData->direction = Normalize(directionalLightData->direction);
					ImGui::EndTabItem();
				}

				break;
			case 2:
				if (ImGui::BeginTabItem("teapot"))
				{
					ImGui::ColorEdit4("color", &teapot.material.materialData->color.x);
					ImGui::DragFloat3("scale", &teapotTransform.scale.x, 0.01f);
					ImGui::DragFloat3("rotate", &teapotTransform.rotate.x, 0.01f);
					ImGui::DragFloat3("translate", &teapotTransform.translate.x, 0.01f);
					ImGui::Checkbox("Lighting", &enableLightting[0]);
					ImGui::Checkbox("useTexture", &useTexture[0]);
					teapot.material.materialData->enabledLighthig = enableLightting[1];
					teapot.material.materialData->useTexture = useTexture[0] ? 1.0f : 0.0f;
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("bunny"))
				{
					ImGui::ColorEdit4("color", &bunny.material.materialData->color.x);
					ImGui::DragFloat3("scale", &bunnyTransform.scale.x, 0.01f);
					ImGui::DragFloat3("rotate", &bunnyTransform.rotate.x, 0.01f);
					ImGui::DragFloat3("translate", &bunnyTransform.translate.x, 0.01f);
					ImGui::Checkbox("Lighting", &enableLightting[1]);
					ImGui::Checkbox("useTexture", &useTexture[1]);
					bunny.material.materialData->enabledLighthig = enableLightting[1];
					bunny.material.materialData->useTexture = useTexture[1] ? 1.0f : 0.0f;
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("DirectionalLight"))
				{
					ImGui::ColorEdit3("color", &directionalLightData->color.x);
					ImGui::DragFloat3("direction", &directionalLightData->direction.x, 0.01f);
					ImGui::DragFloat("intensity", &directionalLightData->intensity, 0.01f);
					ImGui::Checkbox("enable Half Lambert", &ishalf);

					directionalLightData->isHalf = ishalf;
					directionalLightData->direction = Normalize(directionalLightData->direction);
					ImGui::EndTabItem();
				}

				break;
			case 3:
				if (ImGui::BeginTabItem("suzanne"))
				{
					ImGui::ColorEdit4("color", &suzanne.material.materialData->color.x);
					ImGui::DragFloat3("scale", &suzanneTransform.scale.x, 0.01f);
					ImGui::DragFloat3("rotate", &suzanneTransform.rotate.x, 0.01f);
					ImGui::DragFloat3("translate", &suzanneTransform.translate.x, 0.01f);
					ImGui::Checkbox("Lighting", &enableLightting[0]);
					suzanne.material.materialData->enabledLighthig = enableLightting[0];
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("DirectionalLight"))
				{
					ImGui::ColorEdit3("color", &directionalLightData->color.x);
					ImGui::DragFloat3("direction", &directionalLightData->direction.x, 0.01f);
					ImGui::DragFloat("intensity", &directionalLightData->intensity, 0.01f);
					ImGui::Checkbox("enable Half Lambert", &ishalf);

					directionalLightData->isHalf = ishalf;
					directionalLightData->direction = Normalize(directionalLightData->direction);
					ImGui::EndTabItem();
				}
				break;
			case 4:
				if (ImGui::BeginTabItem("multiMesh"))
				{
					auto iteObj1 = multiMesh.mesh.begin();
					auto iteObj2 = multiMesh.mesh.begin();
					std::advance(iteObj2, 1);

					Object obj1 = findObjGroup(multiMesh, (*iteObj1).first);
					Object obj2 = findObjGroup(multiMesh, (*iteObj2).first);

					ImGui::ColorEdit4("color", &obj1.material.materialData->color.x);
					ImGui::DragFloat3("scale", &multiMeshTransform.scale.x, 0.01f);
					ImGui::DragFloat3("rotate", &multiMeshTransform.rotate.x, 0.01f);
					ImGui::DragFloat3("translate", &multiMeshTransform.translate.x, 0.01f);
					ImGui::Checkbox("Lighting", &enableLightting[0]);
					ImGui::Checkbox("useTexture", &useTexture[0]);
					obj1.material.materialData->enabledLighthig = enableLightting[0];
					obj1.material.materialData->useTexture = useTexture[0] ? 1.0f : 0.0f;

					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("DirectionalLight"))
				{
					ImGui::ColorEdit3("color", &directionalLightData->color.x);
					ImGui::DragFloat3("direction", &directionalLightData->direction.x, 0.01f);
					ImGui::DragFloat("intensity", &directionalLightData->intensity, 0.01f);
					ImGui::Checkbox("enable Half Lambert", &ishalf);

					directionalLightData->isHalf = ishalf;
					directionalLightData->direction = Normalize(directionalLightData->direction);
					ImGui::EndTabItem();
				}
				break;
			case 5:
				if (ImGui::BeginTabItem("multiMaterial"))
				{
					auto iteObj1 = multiMaterial.mesh.begin();
					auto iteObj2 = multiMaterial.mesh.begin();
					std::advance(iteObj2, 1);

					Object obj1 = findObjGroup(multiMaterial, (*iteObj1).first);
					Object obj2 = findObjGroup(multiMaterial, (*iteObj2).first);

					ImGui::DragFloat3("scale", &multiMaterialTransform.scale.x, 0.01f);
					ImGui::DragFloat3("rotate", &multiMaterialTransform.rotate.x, 0.01f);
					ImGui::DragFloat3("translate", &multiMaterialTransform.translate.x, 0.01f);

					ImGui::PushID((*iteObj1).first.c_str());
					if (ImGui::CollapsingHeader((*iteObj1).first.c_str()))
					{
						ImGui::ColorEdit4("color", &obj1.material.materialData->color.x);
						ImGui::Checkbox("Lighting", &enableLightting[0]);
						ImGui::Checkbox("useTexture", &useTexture[0]);
						obj1.material.materialData->enabledLighthig = enableLightting[0];
						obj1.material.materialData->useTexture = useTexture[0] ? 1.0f : 0.0f;
						ImGui::Spacing();
						if (ImGui::TreeNode("uvTransform"))
						{
							ImGui::DragFloat2("uvTranslate", &multiMaterialUVTrans1.translate.x, 0.01f, -10.0f, 10.0f);
							ImGui::DragFloat2("uvScale", &multiMaterialUVTrans1.scale.x, 0.01f, -10.0f, 10.0f);
							ImGui::SliderAngle("uvRotate", &multiMaterialUVTrans1.rotate.z);
							obj1.material.materialData->uvTransform = MakeAffineMatrix(multiMaterialUVTrans1.scale, multiMaterialUVTrans1.rotate, multiMaterialUVTrans1.translate);
							ImGui::TreePop();
						}
					}
					ImGui::PopID();
					ImGui::PushID((*iteObj2).first.c_str());
					if (ImGui::CollapsingHeader((*iteObj2).first.c_str()))
					{
						ImGui::ColorEdit4("color", &obj2.material.materialData->color.x);
						ImGui::Checkbox("Lighting", &enableLightting[1]);
						ImGui::Checkbox("useTexture", &useTexture[1]);
						obj2.material.materialData->enabledLighthig = enableLightting[1];
						obj2.material.materialData->useTexture = useTexture[1] ? 1.0f : 0.0f;
						ImGui::Spacing();
						if (ImGui::TreeNode("uvTransform"))
						{
							ImGui::DragFloat2("uvTranslate", &multiMaterialUVTrans2.translate.x, 0.01f, -10.0f, 10.0f);
							ImGui::DragFloat2("uvScale", &multiMaterialUVTrans2.scale.x, 0.01f, -10.0f, 10.0f);
							ImGui::SliderAngle("uvRotate", &multiMaterialUVTrans2.rotate.z);
							obj2.material.materialData->uvTransform = MakeAffineMatrix(multiMaterialUVTrans2.scale, multiMaterialUVTrans2.rotate, multiMaterialUVTrans2.translate);
							ImGui::TreePop();
						}
					}
					ImGui::EndTabItem();
					ImGui::PopID();
				}
				if (ImGui::BeginTabItem("DirectionalLight"))
				{
					ImGui::ColorEdit3("color", &directionalLightData->color.x);
					ImGui::DragFloat3("direction", &directionalLightData->direction.x, 0.01f);
					ImGui::DragFloat("intensity", &directionalLightData->intensity, 0.01f);
					ImGui::Checkbox("enable Half Lambert", &ishalf);

					directionalLightData->isHalf = ishalf;
					directionalLightData->direction = Normalize(directionalLightData->direction);
					ImGui::EndTabItem();
				}
				break;
			default:
				break;
			}


			ImGui::EndTabBar();
			ImGui::End();

			Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 viewProjectionMatrix = viewMatrix * projectionMatrix;

			cameraForGPU->worldPosition = cameraTransform.translate;


			*sprite.transform.transformMat = CalculateSpriteWVPMat(spriteTrans);
			*plane.transform.transformMat = CalculateObjectWVPMat(planeTransform, viewProjectionMatrix);
			*sphere.transform.transformMat = CalculateObjectWVPMat(transform, viewProjectionMatrix);
			*teapot.transform.transformMat = CalculateObjectWVPMat(teapotTransform, viewProjectionMatrix);
			*bunny.transform.transformMat = CalculateObjectWVPMat(bunnyTransform, viewProjectionMatrix);
			*suzanne.transform.transformMat = CalculateObjectWVPMat(suzanneTransform, viewProjectionMatrix);
			*multiMesh.transform.transformMat = CalculateObjectWVPMat(multiMeshTransform, viewProjectionMatrix);
			*multiMaterial.transform.transformMat = CalculateObjectWVPMat(multiMaterialTransform, viewProjectionMatrix);

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
			barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
			//遷移前（現在）のResourceState
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			//遷移後のResourceState
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			//transitionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);


			//描画先のRTVを設定する
			//指定した色で画面算体をクリアする

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeaps[] = { srvDescriptorHeap.Get() };
			commandList->SetDescriptorHeaps(1, descriptorHeaps->GetAddressOf());

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


			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


			switch (scene)
			{
			case 0:
			case 1:
			case 2:
			case 4:
			case 5:
				commandList->SetGraphicsRootSignature(rootSignature.Get());
				commandList->SetPipelineState(graphicsPipelineState.Get());
				commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
				commandList->SetGraphicsRootConstantBufferView(4, cameraResource->GetGPUVirtualAddress());
				break;
			case 3:
				commandList->SetGraphicsRootSignature(rootSignatureForunUV.Get());
				commandList->SetPipelineState(graphicsPipelineStateForunUV.Get());
				commandList->SetGraphicsRootConstantBufferView(2, directionalLightResource->GetGPUVirtualAddress());
				commandList->SetGraphicsRootConstantBufferView(3, cameraResource->GetGPUVirtualAddress());
				break;
			default:
				break;
			}


			switch (scene)
			{
			case 0:
				DrawObj(commandList, plane);
				DrawSprite(commandList, sprite);

				break;
			case 1:
				DrawSphere(commandList, sphere);
				break;
			case 2:
				DrawObj(commandList, bunny);
				DrawObj(commandList, teapot);
				break;
			case 3:
				DrawObj(commandList, suzanne);
				break;
			case 4:
				DrawMultiObj(commandList, multiMesh);
				break;
			case 5:
				DrawMultiObj(commandList, multiMaterial);
				break;
			default:
				break;
			}



			///
			/// 描画ここまで
			/// 

			ImGui::Render();

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

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
			Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[] = { commandList.Get() };
			commandQueue->ExecuteCommandLists(1, commandLists->GetAddressOf());
			//GPUとOSに画面の交換を行うように通知する
			swapChain->Present(1, 0);			//	画面が切り替わる

			/// GPUにSignalを送る
			//Fenceの値の更新
			fenceValue++;
			//GPUがここまでたどり着いたときに，Fenceの値を指定した値に代入するようにSignalを送る
			commandQueue->Signal(fence.Get(), fenceValue);

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
			hr = commandList->Reset(commandAllocator.Get(), nullptr);
			assert(SUCCEEDED(hr));


		}
	}

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();


	DeleteTextures();


	CloseWindow(hwnd);

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

Microsoft::WRL::ComPtr<IDxcBlob> ComplieShader(const std::wstring& _filePath, const wchar_t* _profile, Microsoft::WRL::ComPtr<IDxcUtils>& _dxcUtils, Microsoft::WRL::ComPtr<IDxcCompiler3>& _dxcCompiler, Microsoft::WRL::ComPtr<IDxcIncludeHandler>& _includeHandler)
{
	//hlslファイルを読み込む
	//これからシェーダーをコンパイルする旨をログに出す
	Log(ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", _filePath, _profile)));
	//hlslファイルを読む
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
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
	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
	hr = _dxcCompiler->Compile(
		&shaderSourceBuffer,            // 読み込んだファイル
		arguments,			            // コンパイルオプション
		_countof(arguments),            // コンパイルオプションの数
		_includeHandler.Get(),	        // includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)     // コンパイル結果
	);

	assert(SUCCEEDED(hr));

	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		Log(shaderError->GetStringPointer());
		assert(false);
	}

	//コンパイル結果から実行用のバイナリ部分を取得
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	//成功したログを出す
	Log(ConvertString(std::format(L"Compile Succesed,path:{},profile:{}\n", _filePath, _profile)));
	////もう使わないリソースを解放
	//shaderSource->Release();
	//shaderResult->Release();
	//shaderError->Release();

	//実行用バイナリを返却
	return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device>& _device, size_t _sizeInBytes)
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
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	HRESULT hr = _device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
												  &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
												  IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

	return vertexResource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& _device, int32_t _width, int32_t _height)
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
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = _device->CreateCommittedResource(
		&heapProperties, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // Heapの特別な設定は特になし。
		&resourceDesc, // Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込み状態にしておく
		&depthClearValue, // Clear値の値
		IID_PPV_ARGS(resource.GetAddressOf())); // 作成するResourceポインタへのポインタ

	assert(SUCCEEDED(hr));

	//resource->Release();

	return resource;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device>& _device, D3D12_DESCRIPTOR_HEAP_TYPE _heapType, UINT _numDescriptors, bool _shaderVisible)
{
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;              //viewの情報を格納している場所(Discriptor)の束(配列)
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

Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& _device, const DirectX::TexMetadata& _metadata)
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
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = _device->CreateCommittedResource(
		&heapProperties,//heapの設定
		D3D12_HEAP_FLAG_NONE,//heapの特殊な設定。特になし
		&resourceDesc,//resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST,//データ転送される設定
		nullptr,//clearの最適値 使わないのでnullptr
		IID_PPV_ARGS(resource.GetAddressOf()));//作成するresourceポインタへのポインタ
	assert(SUCCEEDED(hr));

	return resource;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages, const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));


	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device.Get(), intermediateSize);
	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());

	//Tetureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}



D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _descriptorHeap, uint32_t _descriptorSize, uint32_t _index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = _descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (_descriptorSize * _index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _descriptorHeap, uint32_t _descriptorSize, uint32_t _index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = _descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (_descriptorSize * _index);
	return handleGPU;
}

void DeleteTextures()
{
	textures.clear();
}

D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(uint32_t _textureHandle)
{
	assert(textures.size() > _textureHandle);
	return textures[_textureHandle].srvHandlerGPU;
}

uint32_t LoadTexture(const std::string& _filePath, const Microsoft::WRL::ComPtr<ID3D12Device>& _device, const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _srvDescriptorHeap, uint32_t _srvSize)
{
	std::string textureName;
	size_t pos = _filePath.find_last_of('/');

	// 「/」が見つからなかった場合（パスに「/」が含まれていない場合）
	if (pos == std::string::npos) {
		textureName = _filePath;
	}

	// 最後の「/」の次の位置から末尾までの部分
	textureName = _filePath.substr(pos + 1);

	auto it = std::find_if(textures.begin(), textures.end(), [&](const auto& texture) {
		return texture.name == textureName;
						   });

	if (it != textures.end())
	{
		return static_cast<uint32_t>(std::distance(textures.begin(), it));
	}

	textures.push_back(Texture());
	size_t index = textures.size() - 1;
	textures[index].name = textureName;

	DirectX::ScratchImage mipImages = LoadTexture(_filePath);
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	textures[index].resource = CreateTextureResource(_device, metadata);
	textures[index].intermediateResource = UploadTextureData(textures[index].resource, mipImages, _device, _commandList);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ	
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	textures[index].srvHandlerCPU = GetCPUDescriptorHandle(_srvDescriptorHeap, _srvSize, (uint32_t)index + 3);
	textures[index].srvHandlerGPU = GetGPUDescriptorHandle(_srvDescriptorHeap, _srvSize, (uint32_t)index + 3);

	_device->CreateShaderResourceView(textures[index].resource.Get(), &srvDesc, textures[index].srvHandlerCPU);
	return (uint32_t)index;
}

Object LoadObjFile(const std::string& _directoryPath, const std::string& _filename, const Microsoft::WRL::ComPtr<ID3D12Device>& _device, const  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _srvDescriptorHeap, uint32_t _srvSize)
{
	Object modelData;				//構築するmodelData
	std::vector<VertexData> vertices;
	std::vector<Vector4> positions;		//位置
	std::vector<Vector3> normals;		//法線
	std::vector<Vector2> texcoords;		//テクスチャ座標
	std::string line;					//ファイルから読んだ1行を格納するもの

	std::string objName;				// "o" オブジェクト識別用

	bool useTexcoord = false;

	std::ifstream file(_directoryPath + "/" + _filename);
	assert(file.is_open());

	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "o") {
			s >> objName;
		}
		else if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoords.push_back(texcoord);
			useTexcoord = true;
		}
		else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (identifier == "f") {
			VertexData triangle[3];
			//面は三角形限定。その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				//頂点の要素へのIndexは「位置/uV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');// /区切りでインデックスを読んでいく
					if (index == "")
						elementIndices[element] = NULL;
					else
						elementIndices[element] = std::stoi(index);

				}
				//要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position;
				Vector2 texcoord;
				Vector3 normal;

				position = positions[elementIndices[0] - 1];
				position.z *= -1.0f;
				if (useTexcoord)
				{
					texcoord = texcoords[elementIndices[1] - 1];
					texcoord.y = 1.0f - texcoord.y;
				}
				else
					texcoord = { 0 ,0 };
				normal = normals[elementIndices[2] - 1];
				normal.z *= -1.0f;


				triangle[faceVertex] = { position,texcoord,normal };

			}
			vertices.push_back(triangle[2]);
			vertices.push_back(triangle[1]);
			vertices.push_back(triangle[0]);

		}
		else if (identifier == "mtllib")
		{
			std::string mtlFilePath;
			s >> mtlFilePath;
			std::ifstream mtlFile(_directoryPath + "/" + mtlFilePath);
			assert(mtlFile.is_open());
			while (std::getline(mtlFile, line))
			{
				std::istringstream mtls(line);
				mtls >> identifier;

				if (identifier == "map_Kd")
				{
					std::string texturePath;
					mtls >> texturePath;

					std::string path = _directoryPath + '/' + texturePath;
					modelData.textureHandle = LoadTexture(path, _device, _commandList, _srvDescriptorHeap, _srvSize);
				}
			}
		}
	}
	file.close();

	modelData.mesh = InitializeMeshData(_device, vertices);
	modelData.transform = InitializeTransformationData(_device);
	modelData.material = InitilizeMaterialData(_device);

	if (!useTexcoord)
		modelData.textureHandle = -1;

	return modelData;
}
MultiObject LoadMultiObjFile(const std::string& _directoryPath, const std::string& _filename, const Microsoft::WRL::ComPtr<ID3D12Device>& _device, const  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _srvDescriptorHeap, uint32_t _srvSize)
{
	MultiObject modelData;				//構築するmodelData
	std::vector<VertexData> vertices;
	std::vector<Vector4> positions;		//位置
	std::vector<Vector3> normals;		//法線
	std::vector<Vector2> texcoords;		//テクスチャ座標
	std::string line;					//ファイルから読んだ1行を格納するもの

	std::string objName;				// "o" オブジェクト識別用
	std::string useMaterialName;

	int objCount = 0;
	bool useTexcoord = false;

	std::ifstream file(_directoryPath + "/" + _filename);
	assert(file.is_open());

	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;


		if (identifier == "o" && objCount != 0)
		{
			modelData.mesh[objName] = InitializeMeshData(_device, vertices);
			modelData.mesh[objName].useMaterialName = useMaterialName;
		}

		if (identifier == "o") {
			s >> objName;
			objCount++;
		}
		else if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoords.push_back(texcoord);
			useTexcoord = true;
		}
		else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (identifier == "f") {
			VertexData triangle[3];
			//面は三角形限定。その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				//頂点の要素へのIndexは「位置/uV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');// /区切りでインデックスを読んでいく
					if (index == "")
						elementIndices[element] = NULL;
					else
						elementIndices[element] = std::stoi(index);

				}
				//要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position;
				Vector2 texcoord;
				Vector3 normal;

				position = positions[elementIndices[0] - 1];
				position.z *= -1.0f;
				if (useTexcoord)
				{
					texcoord = texcoords[elementIndices[1] - 1];
					texcoord.y = 1.0f - texcoord.y;
				}
				else
					texcoord = { 0 ,0 };
				normal = normals[elementIndices[2] - 1];
				normal.z *= -1.0f;


				triangle[faceVertex] = { position,texcoord,normal };

			}
			vertices.push_back(triangle[2]);
			vertices.push_back(triangle[1]);
			vertices.push_back(triangle[0]);

		}
		else if (identifier == "mtllib")
		{
			std::string mtlName;
			std::string mtlFilePath;
			s >> mtlFilePath;
			std::ifstream mtlFile(_directoryPath + "/" + mtlFilePath);
			assert(mtlFile.is_open());
			while (std::getline(mtlFile, line))
			{
				std::istringstream mtls(line);
				identifier = "0";
				mtls >> identifier;

				if (identifier == "map_Kd")
				{
					std::string texturePath;
					mtls >> texturePath;

					std::string path = _directoryPath + '/' + texturePath;
					modelData.textureHandle[mtlName] = LoadTexture(path, _device, _commandList, _srvDescriptorHeap, _srvSize);
					modelData.material[mtlName] = InitilizeMaterialData(_device);
				}
				else if (identifier == "newmtl")
				{
					mtls >> mtlName;
				}
			}
		}
		else if (identifier == "usemtl") {
			s >> useMaterialName;
		}

	}
	file.close();

	modelData.mesh[objName] = InitializeMeshData(_device, vertices);
	modelData.mesh[objName].useMaterialName = useMaterialName;

	modelData.transform = InitializeTransformationData(_device);

	return modelData;
}

Object findObjGroup(const MultiObject& _obj, std::string _key)
{
	Object obj;

	obj.mesh = (*_obj.mesh.find(_key)).second;
	obj.material = (*_obj.material.find(obj.mesh.useMaterialName)).second;
	obj.textureHandle = (*_obj.textureHandle.find(obj.mesh.useMaterialName)).second;
	obj.transform = _obj.transform;

	return obj;
}

Object MakeTriangleData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device)
{
	Object obj;
	std::vector<VertexData> vertices(3);

	vertices[0].position = { -0.5f, -0.5f, 0.0f, 1.0f };
	vertices[0].texcoord = { 0.0f, 1.0f };
	vertices[0].normal.x = vertices[0].position.x;
	vertices[0].normal.y = vertices[0].position.y;
	vertices[0].normal.z = vertices[0].position.z;

	vertices[1].position = { 0.0f, 0.5f, 0.0f, 1.0f };
	vertices[1].texcoord = { 0.5f, 0.0f };
	vertices[1].normal.x = vertices[1].position.x;
	vertices[1].normal.y = vertices[1].position.y;
	vertices[1].normal.z = vertices[1].position.z;

	vertices[2].position = { 0.5f, -0.5f, 0.0f, 1.0f };
	vertices[2].texcoord = { 1.0f, 1.0f };
	vertices[2].normal.x = vertices[2].position.x;
	vertices[2].normal.y = vertices[2].position.y;
	vertices[2].normal.z = vertices[2].position.z;

	obj.mesh = InitializeMeshData(_device, vertices);
	obj.material = InitilizeMaterialData(_device);
	obj.transform = InitializeTransformationData(_device);
	obj.textureHandle = 0;

	return obj;
}

Object MakeSphereData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device)
{
	Object obj;

	// 分割数
	const uint32_t kSubdivision = 16;
	const uint32_t sphereVertexNum = kSubdivision * kSubdivision * 6;
	std::vector<VertexData> vertices(sphereVertexNum);

	const float kLatEvery = (float)M_PI / (float)kSubdivision;          // 緯度分割1つ分の角度 θ
	const float kLonEvery = (float)M_PI * 2.0f / (float)kSubdivision;    // 経度分割1つ分の角度 φ

	// 緯度の方向に分割   -π/2 ~ π/2
	for (uint32_t latIndex = 0; latIndex < kSubdivision; latIndex++)
	{
		float lat = -(float)M_PI / 2.0f + kLatEvery * latIndex;         // 現在の緯度

		// 経度の方向に分割   0 ~ 2π
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; lonIndex++)
		{
			uint32_t startIndex = (latIndex * kSubdivision + lonIndex) * 6;
			float lon = lonIndex * kLonEvery;                           // 現在の経度

			// a
			vertices[startIndex].position.x = std::cosf(lat) * std::cosf(lon);
			vertices[startIndex].position.y = std::sinf(lat);
			vertices[startIndex].position.z = std::cosf(lat) * std::sinf(lon);
			vertices[startIndex].position.w = 1.0f;
			vertices[startIndex].texcoord.x = float(lonIndex) / float(kSubdivision);
			vertices[startIndex].texcoord.y = 1.0f - float(latIndex) / float(kSubdivision);
			vertices[startIndex].normal.x = vertices[startIndex].position.x;
			vertices[startIndex].normal.y = vertices[startIndex].position.y;
			vertices[startIndex].normal.z = vertices[startIndex].position.z;
			vertices[startIndex].normal = Normalize(vertices[startIndex++].normal);

			// b
			vertices[startIndex].position.x = std::cosf(lat + kLatEvery) * std::cosf(lon);
			vertices[startIndex].position.y = std::sinf(lat + kLatEvery);
			vertices[startIndex].position.z = std::cosf(lat + kLatEvery) * std::sinf(lon);
			vertices[startIndex].position.w = 1.0f;
			vertices[startIndex].texcoord.x = float(lonIndex) / float(kSubdivision);
			vertices[startIndex].texcoord.y = 1.0f - float(latIndex + 1) / float(kSubdivision);
			vertices[startIndex].normal.x = vertices[startIndex].position.x;
			vertices[startIndex].normal.y = vertices[startIndex].position.y;
			vertices[startIndex].normal.z = vertices[startIndex].position.z;
			vertices[startIndex].normal = Normalize(vertices[startIndex++].normal);

			// c
			vertices[startIndex].position.x = std::cosf(lat) * std::cosf(lon + kLonEvery);
			vertices[startIndex].position.y = std::sinf(lat);
			vertices[startIndex].position.z = std::cosf(lat) * std::sinf(lon + kLonEvery);
			vertices[startIndex].position.w = 1.0f;
			vertices[startIndex].texcoord.x = float(lonIndex + 1) / float(kSubdivision);
			vertices[startIndex].texcoord.y = 1.0f - float(latIndex) / float(kSubdivision);
			vertices[startIndex].normal.x = vertices[startIndex].position.x;
			vertices[startIndex].normal.y = vertices[startIndex].position.y;
			vertices[startIndex].normal.z = vertices[startIndex].position.z;
			vertices[startIndex].normal = Normalize(vertices[startIndex++].normal);

			// bコピー
			vertices[startIndex] = vertices[startIndex - 2];
			vertices[startIndex].normal = Normalize(vertices[startIndex++].normal);

			// d
			vertices[startIndex].position.x = std::cosf(lat + kLatEvery) * std::cosf(lon + kLonEvery);
			vertices[startIndex].position.y = std::sinf(lat + kLatEvery);
			vertices[startIndex].position.z = std::cosf(lat + kLatEvery) * std::sinf(lon + kLonEvery);
			vertices[startIndex].position.w = 1.0f;
			vertices[startIndex].texcoord.x = float(lonIndex + 1) / float(kSubdivision);
			vertices[startIndex].texcoord.y = 1.0f - float(latIndex + 1) / float(kSubdivision);
			vertices[startIndex].normal.x = vertices[startIndex].position.x;
			vertices[startIndex].normal.y = vertices[startIndex].position.y;
			vertices[startIndex].normal.z = vertices[startIndex].position.z;
			vertices[startIndex].normal = Normalize(vertices[startIndex++].normal);

			// cコピー
			vertices[startIndex] = vertices[startIndex - 3];
			vertices[startIndex].normal = Normalize(vertices[startIndex].normal);
		}
	}

	obj.mesh = InitializeMeshData(_device, vertices);
	obj.material = InitilizeMaterialData(_device);
	obj.transform = InitializeTransformationData(_device);
	obj.textureHandle = 0;

	return obj;
}


Object MakeSpriteData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device)
{
	Object obj;

	// MeshDataの初期化
	obj.mesh.vertexResource = CreateBufferResource(_device, sizeof(VertexData) * 4);
	obj.mesh.vertexBufferView = { 0 };
	obj.mesh.vertexBufferView.BufferLocation = obj.mesh.vertexResource->GetGPUVirtualAddress();
	obj.mesh.vertexBufferView.SizeInBytes = sizeof(VertexData) * 4;
	obj.mesh.vertexBufferView.StrideInBytes = sizeof(VertexData);

	obj.mesh.indexResource = CreateBufferResource(_device, sizeof(uint32_t) * 6);
	obj.mesh.indexBufferView = { 0 };
	obj.mesh.indexBufferView.BufferLocation = obj.mesh.indexResource->GetGPUVirtualAddress();
	obj.mesh.indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	obj.mesh.indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	obj.mesh.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&obj.mesh.vertexData));
	obj.mesh.indexResource->Map(0, nullptr, reinterpret_cast<void**>(&obj.mesh.indexData));

	// 頂点データの設定
	obj.mesh.vertexData[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; // 左下
	obj.mesh.vertexData[0].texcoord = { 0.0f, 1.0f };
	obj.mesh.vertexData[0].normal = { 0.0f, 0.0f, -1.0f };

	obj.mesh.vertexData[1].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // 左上
	obj.mesh.vertexData[1].texcoord = { 0.0f, 0.0f };
	obj.mesh.vertexData[1].normal = { 0.0f, 0.0f, -1.0f };

	obj.mesh.vertexData[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
	obj.mesh.vertexData[2].texcoord = { 1.0f, 1.0f };
	obj.mesh.vertexData[2].normal = { 0.0f, 0.0f, -1.0f };

	obj.mesh.vertexData[3].position = { 640.0f, 0.0f, 0.0f, 1.0f }; // 右上
	obj.mesh.vertexData[3].texcoord = { 1.0f, 0.0f };
	obj.mesh.vertexData[3].normal = { 0.0f, 0.0f, -1.0f };

	// インデックスデータの設定
	obj.mesh.indexData[0] = 0;
	obj.mesh.indexData[1] = 1;
	obj.mesh.indexData[2] = 2;
	obj.mesh.indexData[3] = 1;
	obj.mesh.indexData[4] = 3;
	obj.mesh.indexData[5] = 2;

	obj.material = InitilizeMaterialData(_device);
	obj.transform = InitializeTransformationData(_device);
	obj.textureHandle = 0;

	return obj;
}

MeshData InitializeMeshData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device, const std::vector<VertexData>& _vertex)
{
	MeshData mesh;

	//頂点リソースを作る
	mesh.vertexResource = CreateBufferResource(_device, sizeof(VertexData) * _vertex.size());
	//頂点バッファビューを作成する
	mesh.vertexBufferView.BufferLocation = mesh.vertexResource->GetGPUVirtualAddress();//リソースの先頭のアドレスから使う
	mesh.vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * _vertex.size());//使用するリソースのサイズは頂点のサイズ
	mesh.vertexBufferView.StrideInBytes = sizeof(VertexData);//1頂点あたりのサイズ

	mesh.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&mesh.vertexData)); //書き込むためのアドレスを取得
	std::memcpy(mesh.vertexData, _vertex.data(), sizeof(VertexData) * _vertex.size());//頂点データをリソースにコピー

	mesh.vertexNum = static_cast<uint32_t> (_vertex.size());

	return mesh;
}

MaterialData InitilizeMaterialData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device)
{
	MaterialData material;

	material.materialResource = CreateBufferResource(_device, sizeof(Material));
	material.materialResource->Map(0, nullptr, reinterpret_cast<void**>(&material.materialData));

	material.materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	material.materialData->enabledLighthig = true;
	material.materialData->uvTransform = MakeIdentity4x4();
	material.materialData->shininess = 40.0f;
	material.materialData->useTexture = 1.0f;

	return material;
}

TransformationData InitializeTransformationData(const Microsoft::WRL::ComPtr<ID3D12Device>& _device)
{
	TransformationData transform;

	transform.wvpResource = CreateBufferResource(_device, sizeof(TransformationMatrix));
	transform.wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&transform.transformMat));

	transform.transformMat->World = MakeIdentity4x4();
	transform.transformMat->worldInverseTranspose = Inverse(transform.transformMat->World);
	transform.transformMat->WVP = MakeIdentity4x4();

	return transform;
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
	transMat.worldInverseTranspose = Transpose(Inverse(transMat.World));
	return TransformationMatrix(transMat);
}

void DrawTriangle(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const Object& _obj)
{
	// MeshDataから頂点バッファビューを取得して設定
	_commandList->IASetVertexBuffers(0, 1, &_obj.mesh.vertexBufferView);

	// MaterialDataからマテリアルリソースのGPU仮想アドレスを設定
	_commandList->SetGraphicsRootConstantBufferView(0, _obj.material.materialResource->GetGPUVirtualAddress());
	// TransformationDataから変換行列リソースのGPU仮想アドレスを設定
	_commandList->SetGraphicsRootConstantBufferView(1, _obj.transform.wvpResource->GetGPUVirtualAddress());

	// テクスチャハンドルを設定
	_commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(_obj.textureHandle));

	// 頂点数3で描画
	_commandList->DrawInstanced(3, 1, 0, 0);
}

void DrawSprite(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const Object& _obj)
{
	_commandList->IASetVertexBuffers(0, 1, &_obj.mesh.vertexBufferView);
	_commandList->IASetIndexBuffer(&_obj.mesh.indexBufferView);

	_commandList->SetGraphicsRootConstantBufferView(0, _obj.material.materialResource->GetGPUVirtualAddress());
	_commandList->SetGraphicsRootConstantBufferView(1, _obj.transform.wvpResource->GetGPUVirtualAddress());

	_commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(_obj.textureHandle));

	_commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void DrawSphere(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const Object& _obj)
{
	_commandList->IASetVertexBuffers(0, 1, &_obj.mesh.vertexBufferView);

	_commandList->SetGraphicsRootConstantBufferView(0, _obj.material.materialResource->GetGPUVirtualAddress());
	_commandList->SetGraphicsRootConstantBufferView(1, _obj.transform.wvpResource->GetGPUVirtualAddress());

	_commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(_obj.textureHandle));

	_commandList->DrawInstanced(_obj.mesh.vertexNum, 1, 0, 0);
}

void DrawObj(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const Object& _model)
{
	_commandList->IASetVertexBuffers(0, 1, &_model.mesh.vertexBufferView);

	if (_model.textureHandle != -1)
	{
		_commandList->SetGraphicsRootConstantBufferView(0, _model.material.materialResource->GetGPUVirtualAddress());
		_commandList->SetGraphicsRootConstantBufferView(1, _model.transform.wvpResource->GetGPUVirtualAddress());
		_commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(_model.textureHandle));
	}
	else
	{
		_commandList->SetGraphicsRootConstantBufferView(0, _model.material.materialResource->GetGPUVirtualAddress());
		_commandList->SetGraphicsRootConstantBufferView(1, _model.transform.wvpResource->GetGPUVirtualAddress());
	}

	_commandList->DrawInstanced(_model.mesh.vertexNum, 1, 0, 0);
}

void DrawMultiObj(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _commandList, const MultiObject& _obj)
{
	Object obj;
	for (const auto& [key, mesh] : _obj.mesh)
	{
		obj = findObjGroup(_obj, key);

		_commandList->IASetVertexBuffers(0, 1, &obj.mesh.vertexBufferView);
		_commandList->SetGraphicsRootConstantBufferView(0, obj.material.materialResource->GetGPUVirtualAddress());
		_commandList->SetGraphicsRootConstantBufferView(1, obj.transform.wvpResource->GetGPUVirtualAddress());
		_commandList->SetGraphicsRootDescriptorTable(2, GetTextureHandle(obj.textureHandle));
		_commandList->DrawInstanced(obj.mesh.vertexNum, 1, 0, 0);
	}
}


Particle MakeNewParticle(std::mt19937& _randomEngine, const Emitter& _emitter)
{

	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	Particle particle;
	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
	Vector3 randomTranslate{ distribution(_randomEngine), distribution(_randomEngine), distribution(_randomEngine) };
	particle.transform.translate = _emitter.transform.translate + randomTranslate;

	particle.velocity = { distribution(_randomEngine), distribution(_randomEngine), distribution(_randomEngine) };



	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
	particle.color = { distColor(_randomEngine), distColor(_randomEngine), distColor(_randomEngine),1.0f };

	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
	particle.lifeTime = distTime(_randomEngine);
	particle.currentTime = 0;


	return particle;
}

std::list<Particle> Emit(const Emitter& _emitter, std::mt19937& _randomEngine)
{
	std::list<Particle> particles;
	for (uint32_t count = 0; count < _emitter.count; count++)
	{
		particles.push_back(MakeNewParticle(_randomEngine, _emitter));
	}
	return particles;
}

void SetBlendMode(BlendMode _blendMode, D3D12_GRAPHICS_PIPELINE_STATE_DESC& _graphicsPipelineStateDesc)
{
	D3D12_BLEND_DESC blendDesc{};
	//すべての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	switch (_blendMode)
	{
	case BlendMode::kBlendModeNormal:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		break;
	case BlendMode::kBlendModeAdd:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		break;
	case BlendMode::kBlendModeSub:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		break;
	case BlendMode::kBlendModeMultiply:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
		break;
	case BlendMode::kBlendModeScreen:
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		break;
	default:
		break;
	}

	_graphicsPipelineStateDesc.BlendState = blendDesc;                                                               // BlendState


}

TransformationMatrix CalculateParticleWVPMat(const stTransform& _transform, const Matrix4x4& _cameraMatrix, const Matrix4x4& _VPmat, bool useBillborad)
{
	TransformationMatrix transMat;

	Matrix4x4 billboardatrix;
	if (useBillborad)
	{
		billboardatrix = _cameraMatrix;
		billboardatrix.m[3][0] = 0;
		billboardatrix.m[3][1] = 0;
		billboardatrix.m[3][2] = 0;
	}
	else
		billboardatrix = MakeIdentity4x4();

	Matrix4x4 scaleMatrix = MakeScaleMatrix(_transform.scale);
	Matrix4x4 translateMatrix = MakeTranslateMatrix(_transform.translate);

	transMat.World = scaleMatrix * billboardatrix * translateMatrix;
	transMat.WVP = transMat.World * _VPmat;

	return TransformationMatrix(transMat);
}
