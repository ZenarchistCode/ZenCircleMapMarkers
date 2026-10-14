#ifdef DZ_Expansion_Navigation
modded class ExpansionMapMarkerList
{
	protected Widget m_ZenCircleCategory;
	protected Widget m_ZenCircleCategoryContent;
	protected ButtonWidget m_ZenCircleCategoryHeader;
	protected ImageWidget m_ZenCircleCategoryArrow;
	protected ButtonWidget m_ZenAllCirclesToggle;
	protected ImageWidget m_ZenAllCirclesIcon;

	protected Widget m_ZenNoBuildCategory;
	protected ButtonWidget m_ZenNoBuildToggle;
	protected ImageWidget m_ZenNoBuildIcon;
	protected int m_ZenNoBuildRevision = -1;
	protected bool m_ZenNoBuildAvailable;
	protected bool m_ZenNoBuildResetPending;

	override void Init(ExpansionMapMenu mapmenu)
	{
		super.Init(mapmenu);

		ZenRefreshCircleCategory();
	}

	void ZenRefreshCircleCategory()
	{
		// Always present, including before sync and with no no-build-zone provider.
		if (!m_ZenCircleCategory)
		{
			if (!m_Root)
				return;

			Widget serverPanel = m_Root.FindAnyWidget("ServerMarkerPanel");
			if (!serverPanel || !serverPanel.GetParent())
				return;

			m_ZenCircleCategory = g_Game.GetWorkspace().CreateWidgets("ZenCircleMapMarkers/GUI/layouts/zen_circle_marker_category.layout", serverPanel.GetParent());
			if (!m_ZenCircleCategory)
				return;

			m_ZenCircleCategoryContent = m_ZenCircleCategory.FindAnyWidget("ZenCircleCategoryContent");
			m_ZenCircleCategoryHeader = ButtonWidget.Cast(m_ZenCircleCategory.FindAnyWidget("ZenCircleCategoryHeaderButton"));
			m_ZenCircleCategoryArrow = ImageWidget.Cast(m_ZenCircleCategory.FindAnyWidget("ZenCircleCategoryHeaderIcon"));
			if (!m_ZenCircleCategoryContent || !m_ZenCircleCategoryHeader || !m_ZenCircleCategoryArrow)
			{
				m_ZenCircleCategory.Unlink();
				m_ZenCircleCategory = null;
				return;
			}

			m_ZenCircleCategoryHeader.SetHandler(this);
			m_ZenCircleCategoryArrow.LoadImageFile(0, ExpansionIcons.GetPath("Arrow"));
			m_ZenCircleCategory.SetSort(-1);
		}

		if (!m_ZenAllCirclesToggle)
			ZenCreateCircleToggle("All Circle Markers", m_ZenAllCirclesToggle, m_ZenAllCirclesIcon);

		if (m_ZenAllCirclesIcon)
			m_ZenAllCirclesIcon.SetImage(ZenCircleMapMarkerClientData.Get().AreCirclesVisible());
	}

	Widget ZenCreateCircleToggle(string label, out ButtonWidget toggle, out ImageWidget icon)
	{
		if (!m_ZenCircleCategoryContent)
			return null;

		Widget row = g_Game.GetWorkspace().CreateWidgets("ZenCircleMapMarkers/GUI/layouts/zen_circle_marker_toggle.layout", m_ZenCircleCategoryContent);
		if (!row)
			return null;

		toggle = ButtonWidget.Cast(row.FindAnyWidget("ZenCircleToggle"));
		icon = ImageWidget.Cast(row.FindAnyWidget("ZenCircleToggleIcon"));
		TextWidget text = TextWidget.Cast(row.FindAnyWidget("ZenCircleToggleLabel"));
		if (!toggle || !icon || !text)
		{
			row.Unlink();
			toggle = null;
			icon = null;
			return null;
		}

		text.SetText(label);
		toggle.SetHandler(this);
		icon.LoadImageFile(0, EXPANSION_ICON_2D_OFF);
		icon.LoadImageFile(1, EXPANSION_ICON_2D_ON);
		m_ZenCircleCategoryContent.Update();
		m_ZenCircleCategory.GetParent().Update();
		m_Root.Update();
		return row;
	}

	void ZenHideNoBuildZones()
	{
		m_ZenNoBuildResetPending = true;
		ZenRefreshNoBuildCategory();
	}

	void ZenRefreshNoBuildCategory()
	{
		ZenRefreshCircleCategory();

		if (m_ZenNoBuildResetPending && m_MarkerModule && m_MarkerModule.GetData())
		{
			m_MarkerModule.GetData().ZenNoBuildZoneInfo().RemoveVisibility(EXPANSION_MARKER_VIS_MAP);
			m_ZenNoBuildResetPending = false;
		}

		ZenCircleMapMarkerClientData circles = ZenCircleMapMarkerClientData.Get();
		if (m_ZenNoBuildRevision != circles.GetRevision())
		{
			m_ZenNoBuildAvailable = false;
			if (circles.IsReady() && circles.IsEnabled())
			{
				// Enforce can throw at foreach when the array is a direct function result.
				array<ref ZenCircleMapMarkerData> markers = circles.GetMarkers();
				if (!markers)
					return;

				foreach (ZenCircleMapMarkerData marker : markers)
				{
					if (!marker)
						continue;

					if (marker.ID == ZenCircleMapMarkerConstants.NO_BUILD_ZONE_CATEGORY_ID || marker.ExpansionMarkerLink == ZenCircleMapMarkerConstants.NO_BUILD_ZONE_LINK)
					{
						m_ZenNoBuildAvailable = true;
						break;
					}
				}
			}
			m_ZenNoBuildRevision = circles.GetRevision();
		}

		if (m_ZenNoBuildAvailable && !m_ZenNoBuildCategory)
		{
			m_ZenNoBuildCategory = ZenCreateCircleToggle("No Build Zones", m_ZenNoBuildToggle, m_ZenNoBuildIcon);
			if (!m_ZenNoBuildCategory)
				return;

			m_ZenNoBuildCategory.SetSort(1);
			m_ZenNoBuildCategory.GetParent().Update();
			m_Root.Update();
		}

		if (m_ZenNoBuildCategory)
		{
			if (m_ZenNoBuildCategory.IsVisible() != m_ZenNoBuildAvailable)
			{
				m_ZenNoBuildCategory.Show(m_ZenNoBuildAvailable);
				m_ZenNoBuildCategory.GetParent().Update();
				m_Root.Update();
			}
			if (m_ZenNoBuildAvailable && m_MarkerModule && m_MarkerModule.GetData())
				m_ZenNoBuildIcon.SetImage(m_MarkerModule.GetData().ZenNoBuildZoneInfo().IsMapVisible());
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_ZenCircleCategoryHeader && w == m_ZenCircleCategoryHeader)
		{
			bool expanded = !m_ZenCircleCategoryContent.IsVisible();
			m_ZenCircleCategoryContent.Show(expanded);
			if (expanded)
				m_ZenCircleCategoryArrow.SetFlags(WidgetFlags.FLIPV);
			else
				m_ZenCircleCategoryArrow.ClearFlags(WidgetFlags.FLIPV);
			m_ZenCircleCategory.GetParent().Update();
			m_Root.Update();
			return true;
		}

		if (m_ZenAllCirclesToggle && w == m_ZenAllCirclesToggle)
		{
			ZenCircleMapMarkerClientData circles = ZenCircleMapMarkerClientData.Get();
			circles.SetCirclesVisible(!circles.AreCirclesVisible());
			ZenRefreshCircleCategory();
			return true;
		}

		if (m_ZenNoBuildToggle && w == m_ZenNoBuildToggle && m_MarkerModule && m_MarkerModule.GetData())
		{
			ExpansionMarkerClientInfo info = m_MarkerModule.GetData().ZenNoBuildZoneInfo();
			info.FlipVisibility(EXPANSION_MARKER_VIS_MAP);
			m_MarkerModule.Refresh();
			ZenRefreshNoBuildCategory();
			return true;
		}
		
		return super.OnClick(w, x, y, button);
	}
};

modded class ExpansionMapMenu
{
	protected float m_ZenNoBuildCategoryRefresh;

	override void OnShow()
	{
		if (m_MarkerList)
			m_MarkerList.ZenHideNoBuildZones();

		super.OnShow();
		if (m_MarkerList)
			m_MarkerList.ZenRefreshNoBuildCategory();
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		m_ZenNoBuildCategoryRefresh += timeslice;
		if (m_ZenNoBuildCategoryRefresh < ZenCircleMapMarkerConstants.LINK_REFRESH_INTERVAL_SECONDS)
			return;

		m_ZenNoBuildCategoryRefresh = 0;
		if (m_MarkerList)
			m_MarkerList.ZenRefreshNoBuildCategory();
	}
};
#endif
