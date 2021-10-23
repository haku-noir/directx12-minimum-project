#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <d3dx12.h>

#include <tchar.h>
#include <vector>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

#define WINDOW_CLASS _T("DX12Minimum")
#define WINDOW_TITLE _T("DX12 Minimum Project")
#define	WINDOW_STYLE WS_OVERLAPPEDWINDOW
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define ASSERT_RES(res, s) \
    do{ \
        if (res != S_OK) { \
            std::cout << "Erorr of res at " << s << std::endl; \
            return -1; \
        } \
    }while(0)
#define ASSERT_PTR(ptr, s) \
    do{ \
        if (ptr == nullptr) { \
            std::cout << "Erorr of ptr at " << s << std::endl; \
            return -1; \
        } \
    }while(0)

size_t AlignmentedSize(size_t size, size_t alignment) {
    return size + alignment - size % alignment;
}

void EnableDebugLayer() {
    ID3D12Debug* debugLayer = nullptr;
    auto res = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
    debugLayer->EnableDebugLayer();
    debugLayer->Release();
}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

#ifdef _DEBUG
int main() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif // _DEBUG
    auto res = CoInitializeEx(0, COINIT_MULTITHREADED);
    ASSERT_RES(res, "CoInitializeEx");

    WNDCLASSEX wndClass = {};
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpfnWndProc = (WNDPROC)WindowProcedure;
    wndClass.lpszClassName = WINDOW_CLASS;
    wndClass.hInstance = GetModuleHandle(nullptr);

    RegisterClassEx(&wndClass);

    RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&wrc, WINDOW_STYLE, false);

    HWND hwnd = CreateWindow(
        wndClass.lpszClassName,
        WINDOW_TITLE,
        WINDOW_STYLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        wndClass.hInstance,
        nullptr
    );

    IDXGIFactory6* dxgiFactory = nullptr;
#ifdef _DEBUG
    EnableDebugLayer();
    res = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory));
#else
    res = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
#endif // _DEBUG

    ID3D12Device* dev = nullptr;
    {
        std::vector<IDXGIAdapter*> adapters;
        IDXGIAdapter* tmpAdapter = nullptr;
        for (int i = 0; dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            adapters.push_back(tmpAdapter);
        }

        for (auto adpt : adapters) {
            DXGI_ADAPTER_DESC adesc = {};
            adpt->GetDesc(&adesc);
            std::wstring strDesc = adesc.Description;

            if (strDesc.find(L"NVIDIA") != std::string::npos) {
                tmpAdapter = adpt;
                break;
            }
        }

        D3D_FEATURE_LEVEL levels[] = {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        D3D_FEATURE_LEVEL featureLevel;
        for (auto lv : levels) {
            if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&dev)) == S_OK) {
                featureLevel = lv;
                break;
            }
        }
        ASSERT_PTR(dev, "D3D12CreateDevice");
    }

    ID3D12CommandAllocator* cmdAllocator = nullptr;
    {
        res = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
        ASSERT_RES(res, "CreateCommandAllocator");
    }

    ID3D12GraphicsCommandList* cmdList = nullptr;
    {
        res = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));
        ASSERT_RES(res, "CreateCommandList");
    }

    ID3D12CommandQueue* cmdQueue = nullptr;
    {
        D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
        cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        cmdQueueDesc.NodeMask = 0;
        cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        res = dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));
        ASSERT_RES(res, "CreateCommandQueue");
    }

    IDXGISwapChain4* swapChain = nullptr;
    {
        DXGI_SWAP_CHAIN_DESC1 swcDesc1 = {};
        swcDesc1.Width = WINDOW_WIDTH;
        swcDesc1.Height = WINDOW_HEIGHT;
        swcDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swcDesc1.Stereo = false;
        swcDesc1.SampleDesc.Count = 1;
        swcDesc1.SampleDesc.Quality = 0;
        swcDesc1.BufferUsage = DXGI_USAGE_BACK_BUFFER;
        swcDesc1.BufferCount = 2;
        swcDesc1.Scaling = DXGI_SCALING_STRETCH;
        swcDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swcDesc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swcDesc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        res = dxgiFactory->CreateSwapChainForHwnd(cmdQueue, hwnd, &swcDesc1, nullptr, nullptr, (IDXGISwapChain1**)&swapChain);
        ASSERT_RES(res, "CreateSwapChainForHwnd");
    }

    std::vector<ID3D12Resource*> backBuffers;
    auto rtvHeapSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    ID3D12DescriptorHeap* rtvHeap = nullptr;
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.NodeMask = 0;
        heapDesc.NumDescriptors = 2;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        res = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap));
        ASSERT_RES(res, "CreateDescriptorHeap");

        DXGI_SWAP_CHAIN_DESC swcDesc = {};
        res = swapChain->GetDesc(&swcDesc);
        ASSERT_RES(res, "GetDesc");
        backBuffers.resize(swcDesc.BufferCount);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        D3D12_CPU_DESCRIPTOR_HANDLE descHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < swcDesc.BufferCount; ++i) {
            res = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
            ASSERT_RES(res, "GetBuffer");
            dev->CreateRenderTargetView(backBuffers[i], &rtvDesc, descHandle);
            descHandle.ptr += rtvHeapSize;
        }
    }

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    struct Vertex {
        XMFLOAT3 pos;
        XMFLOAT2 uv;
    };

    std::vector<Vertex> vertices = {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}},
    };

    ID3D12Resource* vertBuff = nullptr;
    {
        D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(vertices[0]));

        res = dev->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertBuff));
        ASSERT_RES(res, "CreateCommittedResource");
    }
    {
        Vertex* vertMap = nullptr;
        res = vertBuff->Map(0, nullptr, (void**)&vertMap);
        ASSERT_RES(res, "Map");
        std::copy(vertices.begin(), vertices.end(), vertMap);
        vertBuff->Unmap(0, nullptr);
    }

    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    {
        vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
        vbView.StrideInBytes = sizeof(vertices[0]);
        vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(vertices[0]));;
    }

    std::vector<unsigned short> indices = {
        0, 1, 2,
        2, 1, 3
    };

    ID3D12Resource* idxBuff = nullptr;
    {
        D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));

        res = dev->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&idxBuff));
        ASSERT_RES(res, "CreateCommittedResource");
    }
    {
        unsigned short* idxMap = nullptr;
        res = idxBuff->Map(0, nullptr, (void**)&idxMap);
        ASSERT_RES(res, "Map");
        std::copy(indices.begin(), indices.end(), idxMap);
        idxBuff->Unmap(0, nullptr);
    }

    D3D12_INDEX_BUFFER_VIEW ibView = {};
    {
        ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
        ibView.Format = DXGI_FORMAT_R16_UINT;
        ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));
    }

    ID3DBlob* vsBlob = nullptr;
    {
        ID3DBlob* errorBlob = nullptr;
        res = D3DCompileFromFile(
            L"VertexShader.hlsl",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main", "vs_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
            0, &vsBlob, &errorBlob
        );
        if (FAILED(res)) {
            if (res == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
                OutputDebugStringA("File not found");
            }
            else {
                std::string errstr;
                errstr.resize(errorBlob->GetBufferSize());
                std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
                OutputDebugStringA(errstr.c_str());
            }
        }
    }

    ID3DBlob* psBlob = nullptr;
    {
        ID3DBlob* errorBlob = nullptr;
        res = D3DCompileFromFile(
            L"PixelShader.hlsl",
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main", "ps_5_0",
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
            0, &psBlob, &errorBlob
        );
        if (FAILED(res)) {
            if (res == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
                OutputDebugStringA("File not found");
            }
            else {
                std::string errstr;
                errstr.resize(errorBlob->GetBufferSize());
                std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
                OutputDebugStringA(errstr.c_str());
            }
        }
    }

    XMMATRIX mat = XMMatrixIdentity();

    ID3D12Resource* constBuff = nullptr;
    {
        D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(XMMATRIX) + 0xff) & ~0xff);

        res = dev->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constBuff));
        ASSERT_RES(res, "CreateCommittedResource");
    }
    XMMATRIX* matrixMap;
    {
        res = constBuff->Map(0, nullptr, (void**)&matrixMap);
        *matrixMap = mat;
        //constBuff->Unmap(0, nullptr);
    }

    TexMetadata metadata = {};
    ScratchImage scratchImg = {};
    {
        res = LoadFromWICFile(L"textures/myicon.png", WIC_FLAGS_NONE, &metadata, scratchImg);
        ASSERT_RES(res, "LoadFromWICFile");
    }
    auto img = scratchImg.GetImage(0, 0, 0);

    ID3D12Resource* texUploadBuff = nullptr;
    {
        D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height);

        res = dev->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&texUploadBuff));
        ASSERT_RES(res, "CreateCommittedResource");
    }

    ID3D12Resource* texBuff = nullptr;
    {
        D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Format = metadata.format;
        resDesc.Width = metadata.width;
        resDesc.Height = metadata.height;
        resDesc.DepthOrArraySize = metadata.arraySize;
        resDesc.MipLevels = metadata.mipLevels;
        resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        resDesc.SampleDesc.Count = 1;
        resDesc.SampleDesc.Quality = 0;

        res = dev->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texBuff));
        ASSERT_RES(res, "CreateCommittedResource");
    }
    {
        uint8_t* texMap = nullptr;
        res = texUploadBuff->Map(0, nullptr, (void**)&texMap);

        auto srcAddr = img->pixels;
        auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
        for (UINT y = 0; y < img->height; ++y) {
            std::copy_n(srcAddr, img->rowPitch, texMap);
            srcAddr += img->rowPitch;
            texMap += rowPitch;
        }
        texUploadBuff->Unmap(0, nullptr); 
    }

    {
        D3D12_TEXTURE_COPY_LOCATION srcTexLocation = {};
        srcTexLocation.pResource = texUploadBuff;
        srcTexLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcTexLocation.PlacedFootprint.Offset = 0;
        srcTexLocation.PlacedFootprint.Footprint.Width = metadata.width;
        srcTexLocation.PlacedFootprint.Footprint.Height = metadata.height;
        srcTexLocation.PlacedFootprint.Footprint.Depth = metadata.depth;
        srcTexLocation.PlacedFootprint.Footprint.RowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
        srcTexLocation.PlacedFootprint.Footprint.Format = img->format;

        D3D12_TEXTURE_COPY_LOCATION dstTexLocation = {};
        dstTexLocation.pResource = texBuff;
        dstTexLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstTexLocation.SubresourceIndex = 0;

        ID3D12Fence* fence = nullptr;
        UINT fenceVal = 0;
        res = dev->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        ASSERT_RES(res, "CreateFence");

        {
            cmdList->CopyTextureRegion(&dstTexLocation, 0, 0, 0, &srcTexLocation, nullptr);

            D3D12_RESOURCE_BARRIER barrierDesc = {};
            barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrierDesc.Transition.pResource = texBuff;
            barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

            cmdList->ResourceBarrier(1, &barrierDesc);
            cmdList->Close();

            ID3D12CommandList* cmdlists[] = { cmdList };
            cmdQueue->ExecuteCommandLists(1, cmdlists);

            cmdQueue->Signal(fence, ++fenceVal);
        }
        
        if (fence->GetCompletedValue() != fenceVal) {
            auto event = CreateEvent(nullptr, false, false, nullptr);
            fence->SetEventOnCompletion(fenceVal, event);
            if (event != 0) {
                WaitForSingleObject(event, INFINITE);
                CloseHandle(event);
            }
        }

        cmdAllocator->Reset();
        cmdList->Reset(cmdAllocator, nullptr);
    }

    const UINT numDescs = 2;
    auto descHeapSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    ID3D12DescriptorHeap* descHeap = nullptr;
    {
        D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
        descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        descHeapDesc.NodeMask = 0;
        descHeapDesc.NumDescriptors = numDescs;
        descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        res = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descHeap));
        ASSERT_RES(res, "CreateDescriptorHeap");

        auto descHandle = descHeap->GetCPUDescriptorHandleForHeapStart();
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = static_cast<UINT>(constBuff->GetDesc().Width);

            dev->CreateConstantBufferView(&cbvDesc, descHandle);
        }
        descHandle.ptr += descHeapSize;
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = metadata.format;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            
            dev->CreateShaderResourceView(texBuff, &srvDesc, descHandle);
        }
        descHandle.ptr += descHeapSize;
    }

    const UINT numDescRanges = numDescs;
    D3D12_DESCRIPTOR_RANGE descRanges[numDescRanges] = {};
    {
        descRanges[0].NumDescriptors = 1;
        descRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        descRanges[0].BaseShaderRegister = 0;
        descRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        descRanges[1].NumDescriptors = 1;
        descRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        descRanges[1].BaseShaderRegister = 0;
        descRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    const UINT numRootParams = 1;
    D3D12_ROOT_PARAMETER rootParams[numRootParams] = {};
    {
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParams[0].DescriptorTable.pDescriptorRanges = descRanges;
        rootParams[0].DescriptorTable.NumDescriptorRanges = numDescRanges;
    }

    ID3D12RootSignature* rootSignature = {};
    {
        D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

        D3D12_ROOT_SIGNATURE_DESC rootsigDesc = {};
        rootsigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        rootsigDesc.pParameters = rootParams;
        rootsigDesc.NumParameters = numRootParams;
        rootsigDesc.pStaticSamplers = &samplerDesc;
        rootsigDesc.NumStaticSamplers = 1;

        ID3DBlob* rootsigBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;
        res = D3D12SerializeRootSignature(&rootsigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootsigBlob, &errorBlob);
        ASSERT_RES(res, "D3D12SerializeRootSignature");

        res = dev->CreateRootSignature(0, rootsigBlob->GetBufferPointer(), rootsigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
        ASSERT_RES(res, "CreateRootSignature");
        rootsigBlob->Release();
    }

    ID3D12PipelineState* piplinestate = nullptr;
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipelineDesc = {};

        gpipelineDesc.pRootSignature = rootSignature;
        gpipelineDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
        gpipelineDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
        gpipelineDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
        gpipelineDesc.PS.BytecodeLength = psBlob->GetBufferSize();

        gpipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        gpipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

        gpipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        gpipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

        gpipelineDesc.InputLayout.pInputElementDescs = inputLayout;
        gpipelineDesc.InputLayout.NumElements = _countof(inputLayout);

        gpipelineDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        gpipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        gpipelineDesc.NumRenderTargets = 1;
        gpipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        gpipelineDesc.SampleDesc.Count = 1;
        gpipelineDesc.SampleDesc.Quality = 0;

        res = dev->CreateGraphicsPipelineState(&gpipelineDesc, IID_PPV_ARGS(&piplinestate));
        ASSERT_RES(res, "CreateGraphicsPipelineState");
    }

    D3D12_VIEWPORT viewport = {};
    {
        viewport.Width = WINDOW_WIDTH;
        viewport.Height = WINDOW_HEIGHT;
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.MaxDepth = 1.0f;
        viewport.MinDepth = 0.0f;
    }

    D3D12_RECT scissorrect = {};
    {
        scissorrect.top = static_cast<long>(viewport.TopLeftY);
        scissorrect.left = static_cast<long>(viewport.TopLeftX);
        scissorrect.right = static_cast<long>(scissorrect.left + viewport.Width);
        scissorrect.bottom = static_cast<long>(scissorrect.top + viewport.Height);
    }

    ShowWindow(hwnd, SW_SHOW);

    ID3D12Fence* fence = nullptr;
    UINT fenceVal = 0;
    {
        res = dev->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        ASSERT_RES(res, "CreateFence");
    }

    D3D12_RESOURCE_BARRIER barrierDesc = {};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierDesc.Transition.Subresource = 0;

    MSG msg = {};

    float angle = 0.0f;
    auto worldMat = XMMatrixRotationY(angle);
    XMFLOAT3 eye(0, 0, -5);
    XMFLOAT3 target(0, 0, 0);
    XMFLOAT3 up(0, 1, 0);
    auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
    auto projMat = XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT), 1.0f, 10.0f);

    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) {
            break;
        }

        angle += 0.1f;
        worldMat = XMMatrixRotationY(angle);
        *matrixMap = mat * worldMat * viewMat * projMat;

        cmdList->SetGraphicsRootSignature(rootSignature);
        cmdList->SetPipelineState(piplinestate);

        cmdList->SetDescriptorHeaps(1, &descHeap);
        {
            auto descHandle = descHeap->GetGPUDescriptorHandleForHeapStart();
            cmdList->SetGraphicsRootDescriptorTable(0, descHandle);
            descHandle.ptr += descHeapSize * rootParams[0].DescriptorTable.NumDescriptorRanges;
        }

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetVertexBuffers(0, 1, &vbView);
        cmdList->IASetIndexBuffer(&ibView);

        cmdList->RSSetViewports(1, &viewport);
        cmdList->RSSetScissorRects(1, &scissorrect);

        auto bbIdx = swapChain->GetCurrentBackBufferIndex();

        barrierDesc.Transition.pResource = backBuffers[bbIdx];
        barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        cmdList->ResourceBarrier(1, &barrierDesc);

        {
            auto descHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
            descHandle.ptr += bbIdx * rtvHeapSize;
            cmdList->OMSetRenderTargets(1, &descHandle, true, nullptr);

            float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            cmdList->ClearRenderTargetView(descHandle, clearColor, 0, nullptr);
        }

        cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

        barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        cmdList->ResourceBarrier(1, &barrierDesc);

        cmdList->Close();

        ID3D12CommandList* cmdlists[] = { cmdList };
        cmdQueue->ExecuteCommandLists(1, cmdlists);

        cmdQueue->Signal(fence, ++fenceVal);

        if (fence->GetCompletedValue() != fenceVal) {
            auto event = CreateEvent(nullptr, false, false, nullptr);
            fence->SetEventOnCompletion(fenceVal, event);
            if (event != 0) {
                WaitForSingleObject(event, INFINITE);
                CloseHandle(event);
            }
        }

        cmdAllocator->Reset();
        cmdList->Reset(cmdAllocator, nullptr);
        swapChain->Present(1, 0);
    }

    UnregisterClass(wndClass.lpszClassName, wndClass.hInstance);

    return 0;
}