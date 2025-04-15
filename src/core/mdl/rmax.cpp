#include <pch.h>

#include <core/mdl/rmax.h>
#include <core/mdl/stringtable.h>
#include <game/rtech/utils/utils.h>

namespace rmax
{
    // mesh func
    void RMAXMesh::AddVertex(const Vector& pos, const Vector& norm)
    {
        base->AddVertex(pos, norm);

        Vertex_t* const vert = base->GetVertexLast(); // just added, back of vector

        vert->weightIndex = static_cast<int>(base->WeightCount());

        vertexCount++;
    }

    void RMAXMesh::AddColor(const Color32 color)
    {
        base->AddColor(color);
    }

    void RMAXMesh::AddTexcoord(const Vector2D& texcoord)
    {
        base->AddTexcood(texcoord);
    }

    void RMAXMesh::AddWeight(const size_t idx, const int bone, const float weight)
    {
        Vertex_t* const vert = base->GetVertex(static_cast<size_t>(vertexIndex) + idx);

        assert(base->WeightCount() == (vert->weightIndex + vert->weightCount)); // weights are not sequential, bad.

        base->AddWeight(bone, weight);
        vert->weightCount++;
    }

    void RMAXMesh::AddIndice(const uint16_t a, const uint16_t b, const uint16_t c, const uint16_t d)
    {
        base->AddIndice(a, b, c, d);
        indiceCount++;
    }

    // exporter func
	void RMAXExporter::ReserveVertices(const size_t count, const int16_t maxTexcoords, const int16_t maxWeights)
	{
		vertices.reserve(count);
		colors.reserve(count);
		texcoords.reserve(count * maxTexcoords);
		weights.reserve(count * maxWeights);
	}

	void RMAXExporter::AddMesh(const int16_t collectionIndex, const int16_t materialIndex, const int16_t uvCount, const int16_t uvIndices, const bool useColor)
	{
		meshes.emplace_back(this, collectionIndex, materialIndex);
		RMAXMesh* const mesh = GetMeshLast();

		//mesh->vertexCount = 0;
		mesh->vertexIndex = static_cast<int>(vertices.size());

		mesh->colorIndex = useColor ? static_cast<int>(colors.size()) : -1;

		mesh->uvIndex = static_cast<int>(texcoords.size());
		mesh->uvCount = uvCount;
		mesh->uvIndices = uvIndices;

		//mesh->indiceCount = 0;
		mesh->indiceIndex = static_cast<int>(indices.size());

	}

	bool RMAXExporter::ToFile() const
	{
        char* buffer = new char[maxFileSize];
        char* const baseptr = buffer;
        char* curpos = baseptr;

        StringTable stringtable;

        Hdr_t* hdr = reinterpret_cast<Hdr_t*>(curpos);
        memset(hdr, 0, sizeof(Hdr_t));

        hdr->id = rmaxFileId;
        hdr->version = curFmtVersion;
        hdr->version_min = curFmtVersionMin;
        stringtable.AddString(reinterpret_cast<char*>(hdr), &hdr->nameOffset, name);
        stringtable.AddString(reinterpret_cast<char*>(hdr), &hdr->skelNameOffset, nameSkel);

        curpos += sizeof(Hdr_t);

        // add our bones
        hdr->boneOffset = static_cast<int>(curpos - baseptr);
        hdr->boneCount = static_cast<int>(bones.size());

        for (auto& bone : bones)
        {
            Bone_t* newBone = reinterpret_cast<Bone_t*>(curpos);

            newBone->parentIndex = bone.parent;
            newBone->pos = bone.pos;
            newBone->quat = bone.quat;
            newBone->scale = bone.scale;

            stringtable.AddString(reinterpret_cast<char*>(newBone), &newBone->nameOffset, bone.name);

            curpos += sizeof(Bone_t);
        }

        hdr->materialOffset = static_cast<int>(curpos - baseptr);
        hdr->materialCount = static_cast<int>(materials.size());

        for (auto& material : materials)
        {
            Material_t* matl = reinterpret_cast<Material_t*>(curpos);

            matl->textureCount = material.textureCount;
            matl->textures = material.textures;
            stringtable.AddString(reinterpret_cast<char*>(matl), &matl->nameOffset, material.name);

            curpos += sizeof(Material_t);
        }

        hdr->meshCollectionOffset = static_cast<int>(curpos - baseptr);
        hdr->meshCollectionCount = static_cast<int>(collections.size());

        for (auto& collection : collections)
        {
            MeshCollection_t* meshcollection = reinterpret_cast<MeshCollection_t*>(curpos);

            meshcollection->meshCount = collection.meshCount;
            stringtable.AddString(reinterpret_cast<char*>(meshcollection), &meshcollection->nameOffset, collection.name);

            curpos += sizeof(MeshCollection_t);
        }

        if (meshes.size() > 0)
        {
            hdr->meshCount = static_cast<int>(meshes.size());
            hdr->meshOffset = static_cast<int>(curpos - baseptr);

            for (auto& meshData : meshes)
            {
                Mesh_t* mesh = reinterpret_cast<Mesh_t*>(curpos);

                mesh->collectionIndex = meshData.collectionIndex;
                mesh->materialIndex = meshData.materialIndex;

                mesh->vertexCount = meshData.vertexCount;
                mesh->vertexIndex = meshData.vertexIndex;

                mesh->colorIndex = meshData.colorIndex;

                mesh->uvIndex = meshData.uvIndex;
                mesh->uvCount = meshData.uvCount;
                mesh->uvIndices = meshData.uvIndices;

                mesh->indiceCount = meshData.indiceCount;
                mesh->indiceIndex = meshData.indiceIndex;

                curpos += sizeof(Mesh_t);
            }

            // write raw data buffers
            const int vertCount = static_cast<int>(vertices.size());
            hdr->vertexCount = vertCount;
            hdr->vertexOffset = static_cast<int>(curpos - baseptr);
            std::memcpy(curpos, vertices.data(), sizeof(Vertex_t) * vertCount);
            curpos += IALIGN16(sizeof(Vertex_t) * vertCount);

            const int colorCount = static_cast<int>(colors.size());
            hdr->vertexColorCount = colorCount;
            hdr->vertexColorOffset = static_cast<int>(curpos - baseptr);
            std::memcpy(curpos, colors.data(), sizeof(Color32) * colorCount);
            curpos += IALIGN16(sizeof(Color32) * colorCount);

            const int uvCount = static_cast<int>(texcoords.size());
            hdr->vertexUvCount = uvCount;
            hdr->vertexUvOffset = static_cast<int>(curpos - baseptr);
            std::memcpy(curpos, texcoords.data(), sizeof(Vector2D) * uvCount);
            curpos += IALIGN16(sizeof(Vector2D) * uvCount);

            const int weightCount = static_cast<int>(weights.size());
            hdr->vertexWeightCount = weightCount;
            hdr->vertexWeightOffset = static_cast<int>(curpos - baseptr);
            std::memcpy(curpos, weights.data(), sizeof(Weight_t) * weightCount);
            curpos += IALIGN16(sizeof(Weight_t) * weightCount);

            const int indiceCount = static_cast<int>(indices.size());
            hdr->indiceCount = indiceCount;
            hdr->indiceOffset = static_cast<int>(curpos - baseptr);
            std::memcpy(curpos, indices.data(), sizeof(Indice_t) * indiceCount);
            curpos += IALIGN16(sizeof(Indice_t) * indiceCount);
        }

        if (animations.size() > 0)
        {
            const int animCount = static_cast<int>(animations.size());

            hdr->animCount = animCount;
            hdr->animOffset = static_cast<int>(curpos - baseptr);

            Anim_t* anim = reinterpret_cast<Anim_t*>(curpos);
            curpos += sizeof(Anim_t) * animCount;

            for (auto& animData : animations)
            {
                anim->flags = animData.flags;
                anim->frameCount = animData.frameCount;
                anim->frameRate = animData.frameRate;
                stringtable.AddString(reinterpret_cast<char*>(anim), &anim->nameOffset, animData.name);
                
                anim->trackOffset = static_cast<int>(curpos - reinterpret_cast<char*>(anim));
                AnimTrack_t* track = reinterpret_cast<AnimTrack_t*>(curpos);
                curpos += sizeof(AnimTrack_t) * animData.trackCount;

                for (size_t i = 0; i < animData.trackCount; i++)
                {
                    RMAXAnimTrack* const trackData = animData.GetTrack(i);
                    
                    track->flags = trackData->flags;
                    track->boneId = trackData->boneId;
                    track->dataOffset = static_cast<int>(curpos - reinterpret_cast<char*>(track));
                    
                    memcpy(curpos, trackData->data, s_FrameSizeLUT[trackData->flags] * anim->frameCount);
                    curpos += (s_FrameSizeLUT[trackData->flags] * anim->frameCount);

                    track++;
                }

                anim++;
            }
        }

        curpos = stringtable.WriteStrings(curpos);

        // export
        if (!CreateDirectories(path.parent_path()))
        {
            assertm(false, "failed to create directory.");
            return false;
        }

        StreamIO rmaxOut(path.string(), eStreamIOMode::Write);
        rmaxOut.write(buffer, IALIGN16(static_cast<int>(curpos - baseptr)));

        return true;
	}
}