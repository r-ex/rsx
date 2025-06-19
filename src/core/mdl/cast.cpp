#include <pch.h>

#include <core/mdl/cast.h>

namespace cast
{
	// CAST PROPERTY
	// constructor
	CastProperty::CastProperty(CastProperty& propIn) : propertyId(propIn.propertyId), nodeId(propIn.nodeId), propertyNameIndex(propIn.propertyNameIndex), storeType(propIn.storeType), arraySize(propIn.arraySize), formatName(propIn.formatName), formatIndex(propIn.formatIndex), raw(0ull)
	{
		switch (storeType)
		{
		case cast::eCastPropertyStoreType::RAW:
		{
			raw = propIn.raw;
			break;
		}
		case cast::eCastPropertyStoreType::PTR:
		case cast::eCastPropertyStoreType::ALLOC:
		{
			ptr = propIn.ptr;
			propIn.ptr = nullptr;
			break;
		}
		case cast::eCastPropertyStoreType::VECTOR:
		{
			vector = std::move(propIn.vector);
			break;
		}
		default:
			break;
		}
	}

	CastProperty::CastProperty(const CastProperty& propIn) : propertyId(propIn.propertyId), nodeId(propIn.nodeId), propertyNameIndex(propIn.propertyNameIndex), storeType(propIn.storeType), arraySize(propIn.arraySize), formatName(propIn.formatName), formatIndex(propIn.formatIndex), raw(0ull)
	{
		switch (storeType)
		{
		case cast::eCastPropertyStoreType::RAW:
		{
			raw = propIn.raw;
			break;
		}
		case cast::eCastPropertyStoreType::PTR:
		{
			ptr = propIn.ptr;
			break;
		}
		case cast::eCastPropertyStoreType::VECTOR:
		{
			vector = propIn.vector;
			break;
		}
		case cast::eCastPropertyStoreType::ALLOC:
		{
			const size_t size = propertyId == CastPropertyId::String ? (strnlen(reinterpret_cast<const char*>(ptr), MAX_PATH) + 1) : s_CastPropertySize.find(propertyId)->second * arraySize;
			alloc = new char[size];
			memcpy(alloc, propIn.alloc, size);
			break;
		}
		default:
			break;
		}
	}

	// strings?
	CastProperty::CastProperty(const CastPropertyId idIn, const CastId nodeIdIn, const int propNameIn, const uint32_t sizeIn, const void* ptrIn, const bool strFormat, const uint16_t strIndex) : propertyId(idIn), nodeId(nodeIdIn), propertyNameIndex(propNameIn),
		arraySize(sizeIn), alloc(nullptr), storeType(eCastPropertyStoreType::ALLOC), formatName(strFormat), formatIndex(strIndex)
	{
		const size_t size = s_CastPropertySize.find(propertyId)->second;
		alloc = new char[size * arraySize];

		if (nullptr != ptrIn)
			memcpy(alloc, ptrIn, size * arraySize);
	}

	CastProperty::~CastProperty()
	{
		if (storeType == eCastPropertyStoreType::ALLOC)
			delete[] alloc;
	}

	// do name related stuff
	void CastProperty::Name(char* nameBuf, uint16_t& nameSize) const
	{
		const char* const propertyName = s_CastPropListMap.find(nodeId)->second[propertyNameIndex];

		if (!formatName)
		{	
			nameSize = static_cast<uint16_t>(strnlen(propertyName, 16));
			memcpy(nameBuf, propertyName, nameSize);

			return;
		}

		char formatBuf[16]{};
		snprintf(formatBuf, 16, propertyName, formatIndex);
		nameSize = static_cast<uint16_t>(strnlen(formatBuf, 16));
		memcpy(nameBuf, formatBuf, 16);

		return;
	}
	
	void CastProperty::Name(uint16_t& nameSize) const
	{
		const char* const propertyName = s_CastPropListMap.find(nodeId)->second[propertyNameIndex];

		if (!formatName)
		{	
			nameSize = static_cast<uint16_t>(strnlen(propertyName, 16));
			return;
		}

		char formatBuf[16]{};
		snprintf(formatBuf, 16, propertyName, formatIndex);
		nameSize = static_cast<uint16_t>(strnlen(formatBuf, 16));

		return;
	}

	// get the disk size
	const uint32_t CastProperty::Size() const
	{
		size_t size = sizeof(CastPropertyHeader);

		// size of property name
		uint16_t nameSize = 0;
		Name(nameSize);
		size += nameSize;

		size += propertyId == CastPropertyId::String ? (strnlen(reinterpret_cast<const char*>(ptr), MAX_PATH) + 1) : (s_CastPropertySize.find(propertyId)->second * arraySize);

		return static_cast<uint32_t>(size);
	}
	
	// write to disk
	char* CastProperty::Write(char* buf) const
	{
		CastPropertyHeader* propertyHeader = reinterpret_cast<CastPropertyHeader*>(buf);

		propertyHeader->Identifier = propertyId;
		propertyHeader->ArrayLength = arraySize;

		buf += sizeof(CastPropertyHeader);

		// write property name
		Name(buf, propertyHeader->NameSize);
		buf += propertyHeader->NameSize;

		const size_t valueSize = s_CastPropertySize.find(propertyId)->second;
		switch (storeType)
		{
		case eCastPropertyStoreType::RAW:
		{
			assertm(arraySize == 1, "cannot store an array as raw data");
			assertm(propertyId != CastPropertyId::String, "cannot store a string as raw data");

			memcpy(buf, &raw, valueSize);
			buf += valueSize;

			break;
		}
		case eCastPropertyStoreType::PTR:
		case eCastPropertyStoreType::ALLOC:
		{
			const size_t size = propertyId == CastPropertyId::String ? (strnlen(reinterpret_cast<const char*>(ptr), MAX_PATH) + 1) : (valueSize * arraySize);
			memcpy(buf, ptr, size);
			buf += size;

			break;
		}
		case eCastPropertyStoreType::VECTOR:
		{
			assertm(propertyId != CastPropertyId::String, "cannot store an array of strings"); // use ptr!!

			for (auto& data : vector)
			{
				memcpy(buf, data, valueSize);
				buf += valueSize;
			}

			break;
		}
		default:
			break;
		}

		return buf;
	}

	// CAST NODE
	// constructor
	CastNode::CastNode(const CastId idIn, const uint64_t hashIn, const size_t childCount) : nodeId(idIn), hash(hashIn)
	{
		children.resize(childCount);
	}

	// get children
	CastNode* CastNode::GetChild(const uint64_t hashIn, std::vector<CastNode>& nodes)
	{
		auto it = std::find_if(nodes.begin(), nodes.end(), [hashIn](const CastNode& node) -> bool { return node.GetHash() == hashIn; });

		if (it != nodes.end())
		{
			return it._Ptr;
		}

		return nullptr;
	}

	CastNode* CastNode::GetChild(const uint64_t hashIn)
	{
		return GetChild(hashIn, children);
	}

	// add children
	CastNode* CastNode::AddChild(const CastId idIn, const uint64_t hashIn)
	{
		return &children.emplace_back(idIn, hashIn);
	}

	CastNode* CastNode::AddChild(const CastId idIn, const uint64_t hashIn, const size_t childCount)
	{
		return &children.emplace_back(idIn, hashIn, childCount);
	}

	CastNode* CastNode::AddChild(CastNode& node)
	{
		return &children.emplace_back(node);
	}

	CastNode* CastNode::AddChild(const CastNode& node)
	{
		return &children.emplace_back(node);
	}

	// set children
	void CastNode::SetChild(const uint64_t hashIn, CastNode& node)
	{
		GetChild(hashIn)->Set(node);
	}

	void CastNode::SetChild(const int idx, CastNode& node)
	{
		GetChild(idx)->Set(node);
	}

	void CastNode::Set(CastNode& node)
	{
		this->nodeId = node.nodeId;
		this->hash = node.hash;
		this->children = std::move(node.children);
		this->properties = std::move(node.properties);
	}

	// set prop
	void CastNode::SetProperty(const size_t idx, const CastPropertyId idIn, const int propNameIn, const size_t rawIn)
	{
		properties.at(idx) = CastProperty(idIn, nodeId, propNameIn, rawIn);
	}

	void CastNode::SetProperty(const size_t idx, const CastPropertyId idIn, const int propNameIn, const void* ptrIn, const uint32_t sizeIn)
	{
		properties.at(idx) = CastProperty(idIn, nodeId, propNameIn, ptrIn, sizeIn);
	}

	void CastNode::SetProperty(const size_t idx, const CastPropertyId idIn, const int propNameIn, const uint32_t sizeIn, const void* ptrIn)
	{
		properties.at(idx) = CastProperty(idIn, nodeId, propNameIn, sizeIn, ptrIn);
	}

	void CastNode::SetProperty(const size_t idx, const CastPropertyId idIn, const int propNameIn, std::vector<const void*>& vectorIn)
	{
		properties.at(idx) = CastProperty(idIn, nodeId, propNameIn, vectorIn);
	}

	// add prop
	CastProperty* CastNode::AddProperty(const CastPropertyId idIn, const int propNameIn, const size_t rawIn, const bool strFormat, const uint16_t strIndex)
	{
		return &properties.emplace_back(idIn, nodeId, propNameIn, rawIn, strFormat, strIndex);
	}

	CastProperty* CastNode::AddProperty(const CastPropertyId idIn, const int propNameIn, const void* ptrIn, const uint32_t sizeIn, const bool strFormat, const uint16_t strIndex)
	{
		return &properties.emplace_back(idIn, nodeId, propNameIn, ptrIn, sizeIn, strFormat, strIndex);
	}

	CastProperty* CastNode::AddProperty(const CastPropertyId idIn, const int propNameIn, const uint32_t sizeIn, const void* ptrIn, const bool strFormat, const uint16_t strIndex)
	{
		return &properties.emplace_back(idIn, nodeId, propNameIn, sizeIn, ptrIn, strFormat, strIndex);
	}

	CastProperty* CastNode::AddProperty(const CastPropertyId idIn, const int propNameIn, std::vector<const void*>& vectorIn)
	{
		return &properties.emplace_back(idIn, nodeId, propNameIn, vectorIn);
	}

	// get the size on disk
	const uint32_t CastNode::Size() const
	{
		size_t size = sizeof(CastNodeHeader);

		for (auto& prop : properties)
			size += prop.Size();

		for (auto& child : children)
			size += child.Size();

		return static_cast<uint32_t>(size);
	}

	// write to disk
	char* CastNode::Write(char* buf) const
	{
		CastNodeHeader* nodeHeader = reinterpret_cast<CastNodeHeader*>(buf);

		nodeHeader->Identifier = nodeId;
		nodeHeader->NodeSize = Size();
		nodeHeader->NodeHash = hash;
		nodeHeader->ChildCount = static_cast<uint32_t>(children.size());
		nodeHeader->PropertyCount = static_cast<uint32_t>(properties.size());

		buf += sizeof(CastNodeHeader);

		for (auto& prop : properties)
		{
			buf = prop.Write(buf);
		}

		for (auto& child : children)
		{
			buf = child.Write(buf);
		}

		return buf;
	}

	// CASTNODEBONE
	void CastNodeBone::MakeNameParent(const char* name, const int parent)
	{
		bone->AddProperty(CastPropertyId::String, static_cast<int>(CastPropsBone::Name), name, 1u);
		bone->AddProperty(CastPropertyId::Integer32, static_cast<int>(CastPropsBone::Parent_Index), static_cast<size_t>(parent));
	}

	void CastNodeBone::MakeLocalBone(const char* name, const int parent, const Vector* pos, const Quaternion* q)
	{
		bone->ReserveProperties(4);

		MakeNameParent(name, parent);

		bone->AddProperty(CastPropertyId::Vector3, static_cast<int>(CastPropsBone::Local_Position), pos, 1u);
		bone->AddProperty(CastPropertyId::Vector4, static_cast<int>(CastPropsBone::Local_Rotation), q, 1u);
	}

	void CastNodeBone::MakeLocalBone(const char* name, const int parent, const Vector* pos, const Quaternion* q, const Vector* scale)
	{
		MakeLocalBone(name, parent, pos, q);

		bone->ReserveProperties(1);

		bone->AddProperty(CastPropertyId::Vector3, static_cast<int>(CastPropsBone::Scale), scale, 1u);
	}

	void CastNodeBone::MakeGlobalBone(const char* name, const int parent, const Vector* pos, const Quaternion* q)
	{
		bone->ReserveProperties(4);

		MakeNameParent(name, parent);

		bone->AddProperty(CastPropertyId::Vector3, static_cast<int>(CastPropsBone::World_Position), pos, 1u);
		bone->AddProperty(CastPropertyId::Vector4, static_cast<int>(CastPropsBone::World_Rotation), q, 1u);
	}

	void CastNodeBone::MakeGlobalBone(const char* name, const int parent, const Vector* pos, const Quaternion* q, const Vector* scale)
	{
		MakeGlobalBone(name, parent, pos, q);

		bone->ReserveProperties(1);

		bone->AddProperty(CastPropertyId::Vector3, static_cast<int>(CastPropsBone::Scale), scale, 1u);
	}

	void CastNodeBone::MakeBone(const char* name, const int parent, const Vector* pos, const Quaternion* q, bool isGlobal)
	{
		cast::CastNode boneNode(cast::CastId::Bone);
		bone = &boneNode;

		isGlobal ? MakeGlobalBone(name, parent, pos, q) : MakeLocalBone(name, parent, pos, q);

		skel->AddChild(boneNode);
	}

	void CastNodeBone::MakeBone(const char* name, const int parent, const Vector* pos, const Quaternion* q, const Vector* scale, bool isGlobal)
	{
		cast::CastNode boneNode(cast::CastId::Bone);
		bone = &boneNode;

		isGlobal ? MakeGlobalBone(name, parent, pos, q, scale) : MakeLocalBone(name, parent, pos, q, scale);

		skel->AddChild(boneNode);
	}

	// CASTNODECURVE
	void CastNodeCurve::MakeCurveName(const char* name, const CastPropsCurveValue type)
	{
		curve->AddProperty(cast::CastPropertyId::String, static_cast<int>(CastPropsCurve::Node_Name), name, 1u);
		curve->AddProperty(cast::CastPropertyId::String, static_cast<int>(CastPropsCurve::Key_Property_Name), s_CastPropsCurveValue[static_cast<int>(type)], 1u);
	}

	void* CastNodeCurve::MakeCurveKeyFrameBuffer(const size_t numFrames, CastPropertyId& propType)
	{
		propType = CastProperty::ValueMinSize(numFrames);

		const size_t size = s_CastPropertySize.find(propType)->second;

		void* frameBuf = new uint8_t[size * numFrames];

		switch (propType)
		{
		case CastPropertyId::Byte:
		{
			MakeCurveKeyFrameBuffer<uint8_t>(frameBuf, numFrames);
			break;
		}
		case CastPropertyId::Short:
		{
			MakeCurveKeyFrameBuffer<uint16_t>(frameBuf, numFrames);
			break;
		}
		case CastPropertyId::Integer32:
		{
			MakeCurveKeyFrameBuffer<uint32_t>(frameBuf, numFrames);
			break;
		}
		default:
		{
			assertm(false, "invalid frameBuffer type");
			delete[] frameBuf;
			return nullptr;
		}
		}

		return frameBuf;
	}

	// prefer to not use this one!
	void CastNodeCurve::MakeCurveKeyFrames(const size_t numFrames)
	{
		CastPropertyId propType;

		void* frameBuf = MakeCurveKeyFrameBuffer(numFrames, propType);

		curve->AddProperty(propType, static_cast<int>(CastPropsCurve::Key_Frame_Buffer), static_cast<uint32_t>(numFrames), frameBuf);
	}

	void CastNodeCurve::MakeCurveKeyFrames(const void* frameBuf, const size_t numFrames)
	{
		CastPropertyId propType = CastProperty::ValueMinSize(numFrames); // wah

		curve->AddProperty(propType, static_cast<int>(CastPropsCurve::Key_Frame_Buffer), frameBuf, static_cast<uint32_t>(numFrames));
	}

	template<class PropType> void CastNodeCurve::MakeCurveKeyValues(const PropType* const track, const size_t trackLength, const size_t numFrames, const CastPropertyId propId)
	{
		if (trackLength == numFrames)
		{
			curve->AddProperty(propId, static_cast<int>(cast::CastPropsCurve::Key_Value_Buffer), static_cast<uint32_t>(numFrames), track);
		}
		else
		{
			CastProperty* trackProp = curve->AddProperty(propId, static_cast<int>(CastPropsCurve::Key_Value_Buffer), static_cast<uint32_t>(numFrames), nullptr);

			PropType* trackFull = reinterpret_cast<PropType*>(trackProp->GetAllocPtr());

			const float incr = static_cast<float>(trackLength) / static_cast<float>(numFrames); // to avoid diving by zero
			for (size_t i = 0; i < numFrames; i++)
			{
				const size_t trackIdx = static_cast<size_t>(i * incr);
				trackFull[i] = track[trackIdx];
			}
		}
	}

	void CastNodeCurve::MakeCurveMode(const CastPropsCurveMode mode, const float weight)
	{

		curve->AddProperty(CastPropertyId::String, static_cast<int>(CastPropsCurve::Mode), s_CastPropsCurveMode[static_cast<int>(mode)], 1u);

		if (mode == CastPropsCurveMode::MODE_ADDITIVE)
			curve->AddProperty(cast::CastPropertyId::Float, static_cast<int>(CastPropsCurve::Additive_Blend_Weight), FLOAT_AS_UINT(weight));
	}

	void CastNodeCurve::MakeCurveQuaternion(const char* name, const Quaternion* const track, const size_t trackLength, const size_t numFrames, const CastPropsCurveMode mode, const float weight)
	{
		cast::CastNode curveNode(cast::CastId::Curve);
		curve = &curveNode;

		curve->ReserveProperties(6);

		MakeCurveName(name, CastPropsCurveValue::ROT_QUAT);
		MakeCurveKeyFrames(numFrames);
		MakeCurveKeyValues<Quaternion>(track, trackLength, numFrames, CastPropertyId::Vector4);
		MakeCurveMode(mode, weight);

		anim->AddChild(curveNode);
	}

	void CastNodeCurve::MakeCurveQuaternion(const char* name, const Quaternion* const track, const size_t trackLength, const void* frameBuf, const size_t numFrames, const CastPropsCurveMode mode, const float weight)
	{
		cast::CastNode curveNode(cast::CastId::Curve);
		curve = &curveNode;

		curve->ReserveProperties(6);

		MakeCurveName(name, CastPropsCurveValue::ROT_QUAT);
		MakeCurveKeyFrames(frameBuf, numFrames);
		MakeCurveKeyValues<Quaternion>(track, trackLength, numFrames, CastPropertyId::Vector4);
		MakeCurveMode(mode, weight);

		anim->AddChild(curveNode);
	}

	void CastNodeCurve::MakeCurveFloat(const char* name, const float* const track, const size_t trackLength, const size_t numFrames, const CastPropsCurveValue type , const CastPropsCurveMode mode, const float weight)
	{
		cast::CastNode curveNode(cast::CastId::Curve);
		curve = &curveNode;

		curve->ReserveProperties(6);

		MakeCurveName(name, type);
		MakeCurveKeyFrames(numFrames);
		MakeCurveKeyValues<float>(track, trackLength, numFrames, CastPropertyId::Float);
		MakeCurveMode(mode, weight);

		anim->AddChild(curveNode);
	}

	void CastNodeCurve::MakeCurveFloat(const char* name, const float* const track, const size_t trackLength, const void* frameBuf, const size_t numFrames, const CastPropsCurveValue type , const CastPropsCurveMode mode, const float weight)
	{
		cast::CastNode curveNode(cast::CastId::Curve);
		curve = &curveNode;

		curve->ReserveProperties(6);

		MakeCurveName(name, type);
		MakeCurveKeyFrames(frameBuf, numFrames);
		MakeCurveKeyValues<float>(track, trackLength, numFrames, CastPropertyId::Float);
		MakeCurveMode(mode, weight);

		anim->AddChild(curveNode);
	}

	void CastNodeCurve::MakeCurveVector(const char* name, const Vector* const track, const size_t trackLength, const size_t numFrames, const CastPropsCurveValue type , const CastPropsCurveMode mode, const float weight)
	{
		// make our float track
		float* trackFix = new float[3 * trackLength];

		for (size_t i = 0; i < trackLength; i++)
		{
			trackFix[i] = track[i][0];
			trackFix[i + (1 * trackLength)] = track[i][1];
			trackFix[i + (2 * trackLength)] = track[i][2];
		}

		for (size_t i = 0; i < 3; i++)
		{
			MakeCurveFloat(name, &trackFix[trackLength * i], trackLength, numFrames, static_cast<CastPropsCurveValue>(i + static_cast<int>(type)), mode, weight);
		}

		delete[] trackFix;
	}

	void CastNodeCurve::MakeCurveVector(const char* name, const Vector* const track, const size_t trackLength, const void* frameBuf, const size_t numFrames, const CastPropsCurveValue type, const CastPropsCurveMode mode, const float weight)
	{
		// make our float track
		float* trackFix = new float[3 * trackLength];

		for (size_t i = 0; i < trackLength; i++)
		{
			trackFix[i] = track[i][0];
			trackFix[i + (1 * trackLength)] = track[i][1];
			trackFix[i + (2 * trackLength)] = track[i][2];
		}

		for (size_t i = 0; i < 3; i++)
		{
			MakeCurveFloat(name, &trackFix[trackLength * i], trackLength, frameBuf, numFrames, static_cast<CastPropsCurveValue>(i + static_cast<int>(type)), mode, weight);
		}

		delete[] trackFix;
	}

	// CAST HEADER/EXPORTER
	CastNode* CastExporter::GetChild(const uint64_t hash)
	{
		return CastNode::GetChild(hash, rootNodes);
	}

	// export this cast to file
	void CastExporter::ToFile() const
	{
		char* fileBuf = new char[castFileSize] {}; // it would be possible to get the file size before writing, but the cost probably does not outweight just allocating a big buffer
		char* curpos = fileBuf;

		CastHeader* castHeader = reinterpret_cast<CastHeader*>(curpos);

		castHeader->Magic = castFileId;
		castHeader->Version = castFileVersion;
		castHeader->RootNodes = static_cast<uint32_t>(rootNodes.size());
		castHeader->Flags = 0; // what?

		curpos += sizeof(CastHeader);

		for (auto& root : rootNodes)
		{
			curpos = root.Write(curpos);
		}

		if (!CreateDirectories(path.parent_path()))
		{
			assertm(false, "failed to create directory");
			return;
		}

		StreamIO out(path.string(), eStreamIOMode::Write);
		out.write(fileBuf, curpos - fileBuf);

		delete[] fileBuf;
	}
}