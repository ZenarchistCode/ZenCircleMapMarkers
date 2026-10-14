class ZenCircleMapMarkerData
{
	string ID;
	vector Position;
	float RadiusMeters;
	int OutlineColor;
	int FillColor;
	float OutlineWidthPixels;
	int CautionEnabled;
	int EnableOverlap;
	string ExpansionMarkerLink;

	void CopyFrom(ZenCircleMapMarkerData other)
	{
		if (!other)
			return;

		ID = other.ID;
		Position = other.Position;
		RadiusMeters = other.RadiusMeters;
		OutlineColor = other.OutlineColor;
		FillColor = other.FillColor;
		OutlineWidthPixels = other.OutlineWidthPixels;
		CautionEnabled = other.CautionEnabled;
		EnableOverlap = other.EnableOverlap;
		ExpansionMarkerLink = other.ExpansionMarkerLink;
	}

	bool HasExpansionLink()
	{
		return ExpansionMarkerLink != "";
	}
};
