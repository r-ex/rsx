#include <pch.h>
#include <core/render/dx.h>
#include <imgui.h>
#include <game/rtech/utils/utils.h>
#include <misc/imgui_utility.h>
#include <core/render/preview/preview.h>
#include <core/input/input.h>

#define IMOGUIZMO_LEFT_HANDED
#include <thirdparty/imgui/misc/imoguizmo.hpp>
#include <core/mdl/modeldata.h>

extern CDXParentHandler* g_dxHandler;

void Preview_Model(CDXDrawData* drawData, float dt)
{
    UpdateModelBoneMatrix(drawData);

    ID3D11Device* const device = g_dxHandler->GetDevice();
    ID3D11DeviceContext* const ctx = g_dxHandler->GetDeviceContext();
    CDXScene& scene = g_dxHandler->GetScene();

    const ImVec2 avail = ImGui::GetContentRegionAvail();

    ID3D11RenderTargetView* const previewRTV = g_dxHandler->GetPreviewRTV();
    static constexpr float clear_color_with_alpha[4] = { 0.01f, 0.01f, 0.01f, 1.00f };

    ctx->OMSetRenderTargets(1, &previewRTV, g_dxHandler->GetPreviewDSV());
    ctx->ClearRenderTargetView(previewRTV, clear_color_with_alpha);
    ctx->ClearDepthStencilView(g_dxHandler->GetPreviewDSV(), D3D11_CLEAR_DEPTH, 1, 0);

    const D3D11_VIEWPORT vp = {
        0, 0,
        static_cast<float>(avail.x),
        static_cast<float>(avail.y),
        0, 1
    };

    ctx->RSSetViewports(1u, &vp);
    ctx->RSSetState(g_dxHandler->GetRasterizerState());
    ctx->OMSetDepthStencilState(g_dxHandler->GetDepthStencilState(true), 1u);

#if (ADVANCED_MODEL_PREVIEW)
    // Update CBufCommonPerCamera
    g_dxHandler->GetCamera()->CommitCameraDataBufferUpdates();

    scene.UpdateHardwareLights();
    scene.UpdateCubemapSamples();

    if (scene.NeedsLightingUpdate())
        scene.MapAndUpdateLightBuffer(device, ctx);

    if (scene.NeedsCubemapSmpUpdate())
        scene.MapAndUpdateCubemapSamplesBuffer(device, ctx);
#else
    UNUSED(device);
#endif

    drawData->SetPSResource(PSRSRC_CUBEMAP, g_dxHandler->GetCubemapSRV());
    drawData->SetPSResource(PSRSRC_CSMDEPTHATLASSAMPLER, g_dxHandler->GetCSMDepthAtlasSamplerSRV());
    drawData->SetPSResource(PSRSRC_SHADOWMAP, g_dxHandler->GetShadowMapSRV());
    drawData->SetPSResource(PSRSRC_CLOUDMASK, g_dxHandler->GetCloudMaskSRV());
    drawData->SetPSResource(PSRSRC_STATICSHADOWTEXTURE, g_dxHandler->GetStaticShadowTexSRV());

    CDXCamera* const camera = g_dxHandler->GetCamera();

    if (drawData->vertexShader && drawData->pixelShader) LIKELY
    {
        assertm(drawData->transformsBuffer, "uh oh something very bad happened!!!!!!");

        ctx->VSSetConstantBuffers(0u, 1u, &drawData->transformsBuffer); // VS_TransformConstants/CBufModelInstance

        scene.previewGrid.Draw(ctx);

        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        UINT offset = 0u;

        for (size_t i = 0; i < drawData->meshBuffers.size(); ++i)
        {
            const DXMeshDrawData_t& meshDrawData = drawData->meshBuffers[i];

            if (!meshDrawData.visible || !meshDrawData.vertexShader || !meshDrawData.pixelShader)
                continue;

            // Add mesh weights StructuredBuffer as a resource
            drawData->SetVSResource(61u, meshDrawData.weightsSRV);

            ctx->RSSetState(meshDrawData.wireframe ? g_dxHandler->GetRasterizerStateWireFrame() : g_dxHandler->GetRasterizerState());

            assertm(meshDrawData.vertexShader != nullptr, "No vertex shader?");
            assertm(meshDrawData.pixelShader != nullptr, "No pixel shader?");

            ctx->IASetInputLayout(meshDrawData.inputLayout);
            ctx->VSSetShader(meshDrawData.vertexShader, nullptr, 0u);

            ID3D11Buffer* sharedConstBuffers[] = {
                camera->bufCommonPerCamera,    // CBufCommonPerCamera - b2
                drawData->modelInstanceBuffer, // CBufModelInstance   - b3
            };

            for (auto& rsrc : drawData->vertexShaderResources)
            {
                if (rsrc.second)
                    ctx->VSSetShaderResources(rsrc.first, 1u, &rsrc.second);
            }

            // [AMP]
            if (meshDrawData.hasGameShaders)
            {
                // VertexShader: CBufCommonPerCamera, CBufModelInstance
                ctx->VSSetConstantBuffers(2u, ARRSIZE(sharedConstBuffers), sharedConstBuffers);
            }

            // VertexShader: g_boneMatrix, g_boneMatrixPrevFrame
            ctx->VSSetShaderResources(VSRSRC_BONE_MATRIX, 1u, &drawData->boneMatrixSRV);
            ctx->VSSetShaderResources(VSRSRC_BONE_MATRIX_PREV_FRAME, 1u, &drawData->boneMatrixSRV);

            ctx->IASetVertexBuffers(0u, 1u, &meshDrawData.vertexBuffer, &meshDrawData.vertexStride, &offset);
            // ==============================================================================

            ctx->PSSetShader(meshDrawData.pixelShader, nullptr, 0u);

            ID3D11SamplerState* const samplerState = g_dxHandler->GetSamplerState();

            // [AMP] Samplers, Lights, CBufs
            if (meshDrawData.hasGameShaders)
            {
                ID3D11SamplerState* samplers[] = {
                    g_dxHandler->GetSamplerComparisonState(),
                    samplerState,
                    samplerState,
                };
                ctx->PSSetSamplers(0, ARRSIZE(samplers), samplers);

                if (meshDrawData.uberStaticBuf)
                    ctx->PSSetConstantBuffers(0u, 1u, &meshDrawData.uberStaticBuf);

                if (meshDrawData.uberDynamicBuf)
                    ctx->PSSetConstantBuffers(1u, 1u, &meshDrawData.uberDynamicBuf);

                // PixelShader: CBufCommonPerCamera, CBufModelInstance
                ctx->PSSetConstantBuffers(2u, ARRSIZE(sharedConstBuffers), sharedConstBuffers);

                // PixelShader: s_globalLights
                ctx->PSSetShaderResources(PSRSRC_GLOBAL_LIGHTS, 1u, &scene.globalLightsSRV);
                ctx->PSSetShaderResources(PSRSRC_CUBEMAP_SAMPLES, 1u, &scene.cubemapSamplesSRV);
            }
            else
                ctx->PSSetSamplers(0, 1, &samplerState);

            // Bind texture resources for this mesh's material
            for (auto& tex : meshDrawData.textures)
            {
                ID3D11ShaderResourceView* const textureSRV = tex.texture
                    ? tex.texture.get()->GetSRV()
                    : nullptr;

                ctx->PSSetShaderResources(tex.resourceBindPoint, 1u, &textureSRV);
            }

            // Bind pixel shader resources
            for (auto& rsrc : drawData->pixelShaderResources)
            {
                ctx->PSSetShaderResources(rsrc.first, 1u, &rsrc.second);
            }

            // ==============================================================================
            ctx->IASetIndexBuffer(meshDrawData.indexBuffer, meshDrawData.indexFormat, 0u);
            ctx->DrawIndexed(static_cast<UINT>(meshDrawData.numIndices), 0u, 0u);
        }

        CShader* vertexShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("preview/prim_vs", s_PrimitiveVertexShader, eShaderType::Vertex, s_PrimitiveInputLayout, std::size(s_PrimitiveInputLayout));
        CShader* pixelShader = g_dxHandler->GetShaderManager()->LoadShaderFromString("preview/prim_ps", s_PrimitivePixelShader, eShaderType::Pixel);
        
        ctx->VSSetConstantBuffers(0u, 1u, &drawData->transformsBuffer);

        auto iterator = drawData->debugPrims.begin();
        while (iterator != drawData->debugPrims.end())
        {
            if (!iterator->visible)
            {
                iterator++;
                continue;
            }

            ctx->IASetPrimitiveTopology(iterator->primTopology);

            ctx->RSSetState(iterator->wireframe ? g_dxHandler->GetRasterizerStateWireFrame() : g_dxHandler->GetRasterizerState());

            ctx->IASetInputLayout(vertexShader->GetInputLayout());
            ctx->VSSetShader(vertexShader->Get<ID3D11VertexShader>(), nullptr, 0u);
            ctx->PSSetShader(pixelShader->Get<ID3D11PixelShader>(), nullptr, 0u);

            constexpr UINT vertexStride = sizeof(PrimitiveVertex_t);
            ctx->IASetVertexBuffers(0u, 1u, &iterator->vertexBuffer, &vertexStride, &offset);

            // ==============================================================================
            ctx->IASetIndexBuffer(iterator->indexBuffer, DXGI_FORMAT_R16_UINT, 0u);

            ctx->OMSetDepthStencilState(g_dxHandler->GetDepthStencilState(false), 1u);

            if (iterator->indexed)
                ctx->DrawIndexed(iterator->numIndices, 0u, 0u);
            else
                ctx->Draw(iterator->numVertices, 0u);

            iterator->lifeRemaining -= dt;

            if (iterator->lifeRemaining <= 0)
                iterator = drawData->debugPrims.erase(iterator);
            else
                iterator++;
        }
    }
    else assertm(0, "Failed to load shaders for model preview.");

    if (g_dxHandler->GetPreviewFrameBuffer())
    {
        const ImVec2 windowPos = ImGui::GetWindowPos();
        const ImVec2 windowSize = ImGui::GetWindowSize();
        const ImVec2 initCursorPos = ImGui::GetCursorPos();

        ImGui::Image(g_dxHandler->GetPreviewFrameBufferSRV(), avail);

        const bool isSceneHovered = ImGui::IsItemHovered();
        if (isSceneHovered)
        {
            const float wheel = ImGui::GetIO().MouseWheel;

            if (wheel != 0.0f)
            {
                const float scrollZoomFactor = ImGui::GetIO().KeyAlt ? (1.f / 5.f) : 2.f;
                camera->distanceToPivot -= (wheel * scrollZoomFactor);
                camera->distanceToPivot = std::clamp(camera->distanceToPivot, 5.f, 300.f);
            }
        }

        // Other UI stuff
        // Calculate the full size of the model name's text.
        // If the full model path is too long for the window, truncate it to just the file name
        const ImVec2 fullTextSize = ImGui::CalcTextSize(drawData->modelName);

        ImGui::SetCursorPos(initCursorPos + ImVec2(3.f, 0.f));
        ImGui::Text("%s", fullTextSize.x > windowSize.x ? GetStringAfterLastSlash(drawData->modelName) : drawData->modelName);

        ImGui::SetCursorPos(ImVec2(initCursorPos.x + 5.f, (initCursorPos.y + windowSize.y) - 215.f));
        ImGui::VSliderFloat("##ModelZoom", ImVec2(20.f, 150.f), &camera->distanceToPivot, 300.f, 5.f, "", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoInput);

        const bool isSliderHovered = ImGui::IsItemHovered();

        ImGui::SetCursorPos(ImVec2(initCursorPos.x + 11.f, (initCursorPos.y + windowSize.y) - 215.f));
        ImGui::Text("+");
        ImGui::SetCursorPos(ImVec2(initCursorPos.x + 13.f, (initCursorPos.y + windowSize.y) - 85.f));
        ImGui::Text("-");

        ImGui::Text("%.f%%", (CAMERA_DEFAULT_DISTANCE / camera->distanceToPivot) * 100.f);

        const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        if (!g_pInput->applyMouseInput)
            g_pInput->applyMouseInput = isSceneHovered && !isSliderHovered && mouseDown;
        else
            g_pInput->applyMouseInput = mouseDown;

        // Gizmo
        auto& cfg = ImOGuizmo::config;

        cfg.positiveRadiusScale = 0.175f;
        cfg.negativeRadiusScale = 0.125f;

        constexpr float squareSize = 45.f;

        const ImVec2 rect(
            windowPos.x + (windowSize.x - (squareSize * 2.f)),
            windowPos.y + (squareSize + 15.f)
        );

        ImOGuizmo::SetRect(rect.x, rect.y, squareSize);

        XMMATRIX viewMatrix = camera->GetViewMatrix();
        ImOGuizmo::DrawGizmo((float*)&viewMatrix, g_dxHandler->GetProjMatrixFloat());
    }
}