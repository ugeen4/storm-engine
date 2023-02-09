#include "log_and_action.h"
#include "controls.h"
#include "core.h"
#include "message.h"
#include "string_compare.hpp"
#include "shared/battle_interface/log_msg.h"
#include <stdio.h>

#define BI_FAST_COMMANDS "BIFastCommand"

//#define SPECIAL_VERSION

void CalculateTexturePos(FRECT &texRect, int hort, int vert, int numt)
{
    const auto vn = numt / hort;
    const auto hn = numt - vn * hort;
    texRect.right = (texRect.left = static_cast<float>(hn) / hort) + 1.f / hort;
    texRect.bottom = (texRect.top = static_cast<float>(vn) / vert) + 1.f / vert;
}

ILogAndActions::ILogAndActions()
{
    rs = nullptr;
    m_idIconTexture = -1;
    m_sRoot = nullptr;
    m_fontID = -1;
    m_fFontScale = 1.f;
    m_bShowActiveCommand = false;
    m_bShowLogStrings = false;
    m_bThatRealAction = false;
    m_nStringBegin = 0;
    m_bDontShowAll = false;
    m_sOldActionName[0] = 0;
    m_nTimeCounter = 0;
	
	//picinfo
	m_pRoot = nullptr;
    m_PicInfofontID = -1;
    m_fPicInfoFontScale = 1.f;
	m_nPicInfoStringBegin = 0;
	
	// TimeSpeed
	bShowTimeSpeed = false;
	
	// level up
	m_idLUBackTexture = -1;
	AlphaLU = 0.f;
}

ILogAndActions::~ILogAndActions()
{
    Release();
}

bool ILogAndActions::Init()
{
    if ((rs = static_cast<VDX9RENDER *>(core.GetService("dx9render"))) == nullptr)
    {
        throw std::runtime_error("Can`t create render service");
    }
    D3DVIEWPORT9 vp;
    rs->GetViewport(&vp);
    core.Event("SetWindowSize", "lll", static_cast<int32_t>(vp.Width), static_cast<int32_t>(vp.Height), false);
    return true;
}

void ILogAndActions::Execute(uint32_t delta_time)
{
    if (m_bDontShowAll)
        return;

    CONTROL_STATE cs;
    core.Controls->GetControlState(BI_FAST_COMMANDS, cs);
    if (cs.state == CST_ACTIVATED)
        core.Event("BI_FastCommand", "s", m_sActionName);

    // fade out lines
    const auto colDelta = delta_time * m_fBlendSpeed;
    STRING_DESCR *prev_sd = nullptr;
    STRING_DESCR *sd;
    for (sd = m_sRoot; sd != nullptr;)
    {
        if (sd->alpha <= 255.f)
            if ((sd->alpha -= colDelta) <= 0)
            {
                if (prev_sd == nullptr)
                    m_sRoot = sd->next;
                else
                    prev_sd->next = sd->next;
                STORM_DELETE(sd->str);
                STORM_DELETE(sd);
                sd = m_sRoot;
                continue;
            }
        prev_sd = sd;
        sd = sd->next;
    }

    // move lines to free positions
    const auto delta = delta_time * m_fShiftSpeed;
    auto top = 0.f;
    for (sd = m_sRoot; sd != nullptr; sd = sd->next)
    {
        if (sd->offset > top)
        {
            sd->offset -= delta;
            if (sd->offset < top)
                sd->offset = top;
        }
        top += m_nStringOffset;
    }
	
	// pic info string
	
	// fade out lines
    const auto colDelta1 = delta_time * m_fPicInfoBlendSpeed;
    STRING_DESCR *prev_pd = nullptr;
    STRING_DESCR *pd;
    for (pd = m_pRoot; pd != nullptr;)
    {
        if (pd->alpha <= 255.f)
            if ((pd->alpha -= colDelta1) <= 0)
            {
                if (prev_pd == nullptr)
                    m_pRoot = pd->next;
                else
                    prev_pd->next = pd->next;
                STORM_DELETE(pd->str);
                STORM_DELETE(pd);
                pd = m_pRoot;
                continue;
            }
        prev_pd = pd;
        pd = pd->next;
    }
	
	
	if ((AlphaBI -= colDelta1) <= 0) AlphaBI = 0.f;
	if ((AlphaBI2 -= colDelta1) <= 0) AlphaBI2 = 0.f;
	if ((AlphaBI3 -= colDelta1) <= 0) AlphaBI3 = 0.f;
	if (m_pRoot == nullptr) PIstage = 0;
	const auto colDelta2 = delta_time * m_fLUStringSpeed;
	if ((AlphaLU -= colDelta2) <= 0) AlphaLU = 0.f;
	
}

uint64_t ILogAndActions::ProcessMessage(MESSAGE &message)
{
    switch (message.Long())
    {
    case LOG_ADD_STRING: {
        const auto stringImmortal = message.Long() != 0;
        const std::string &param = message.String();
        if (stringImmortal)
        {
            // find the last element of the list
            STRING_DESCR *last;
            for (last = m_sRoot; last != nullptr; last = last->next)
                if (last->alpha > 255.f)
                    break;
            if (last == nullptr)
                SetString(param.c_str(), true);
            else
            {
                STORM_DELETE(last->str);
                if (param[0] != 0)
                {
                    const auto len = param.size() + 1;
                    if ((last->str = new char[len]) == nullptr)
                    {
                        throw std::runtime_error("allocate memory error");
                    }
                    strcpy_s(last->str, len, param.c_str());
                }
                else
                {
                    if (last == m_sRoot)
                        m_sRoot = m_sRoot->next;
                    else
                    {
                        for (auto *prev = m_sRoot; prev != nullptr && prev->next != last; prev = prev->next)
                            if (prev != nullptr && prev->next == last) //~!~
                            {
                                prev->next = last->next;
                                break;
                            }
                    }
                    STORM_DELETE(last);
                }
            }
        }
        else
            SetString(param.c_str(), false);
    }
    break;
	
	case LOG_ADD_PIC: 
	{
		if(PIstage == 3) break;
        const std::string &param = message.String();
		m_nPicInfoIconIndex = message.Long();
        SetPicInfo(param.c_str());
    }
    break;
	
	case LOG_TIME_SPEED: 
	{
        TimeSpeedText = message.String();
		bShowTimeSpeed = message.Long() != 0 ;
		if(!bShowTimeSpeed) break;
    }
    break;
	
	case LOG_LEVEL_UP: 
	{
        LUStringText = message.String();
        LURankText = message.String();
        SetLevelUp();
    }
    break;
	
    case LOG_SET_ACTIVE_ACTION: {
        const std::string &param = message.String();
        SetAction(param.c_str());
    }
    break;
    case LOG_AND_ACTIONS_INIT: {
        m_sOldActionName[0] = '0';
        const auto bfc = message.Long();
        const auto bls = message.Long();
        Create(bfc != 0, bls != 0);
    }
    break;
    case LOG_AND_ACTIONS_CHANGE: {
        m_sOldActionName[0] = '0';
        const auto bfc = message.Long();
        const auto bls = message.Long();
        ActionChange(bfc != 0, bls != 0);
    }
    break;
    case LI_SET_VISIBLE:
        m_bDontShowAll = (message.Long() == 0);
        break;
    case LI_CLEAR_STRINGS:
        while (m_sRoot != nullptr)
        {
            auto *const p = m_sRoot;
            m_sRoot = p->next;
            STORM_DELETE(p->str);
            delete p;
        }
        break;
    case LI_OTHER_MSG: {
        const std::string &param = message.String();
        if (storm::iEquals(param, "SetTimeScale"))
        {
            core.SetTimeScale(message.Float());
        }
    }
    break;
    }
    return 0;
}

void ILogAndActions::Realize(uint32_t delta_time)
{
#ifdef SPECIAL_VERSION
    if (core.Controls->GetDebugAsyncKeyState(VK_F8) >= 0)
    {
        m_nTimeCounter += core.GetDeltaTime();
        if (m_nTimeCounter > 10000)
        {
            m_nTimeCounter = 0;
        }
        int32_t nA = 0;
        if (m_nTimeCounter < 500)
        {
            nA = (int32_t)(255.f * m_nTimeCounter / 500.f);
        }
        else if (m_nTimeCounter < 2000)
        {
            nA = 255;
        }
        else if (m_nTimeCounter < 2500)
        {
            nA = (int32_t)(255.f * (2500 - m_nTimeCounter) / 500.f);
        }
        rs->ExtPrint(m_fontID, ARGB(nA, 255, 255, 255), 0, PR_ALIGN_CENTER, false, 3.9f, 800, 600, 400, 300,
                     "¬≈–—»я ƒЋя ѕ–≈——џ");
    }
#endif
    if (core.Controls->GetDebugAsyncKeyState('K') < 0)
        return;
    if (rs == nullptr)
        return;
    if (m_bDontShowAll)
        return;

    rs->MakePostProcess();

    // Show Active Action
    //---------------------
    if (m_bShowActiveCommand)
    {
        CMatrix matw;
        rs->SetTransform(D3DTS_WORLD, matw);
		//backimage
		if (m_idActionBackTexture != -1L && m_bThatRealAction && rs->StringWidth(m_ActionHint2.sText.c_str(), m_ActionHint2.nFont, m_ActionHint2.fScale, 0) > 1)
		{
			rs->TextureSet(0, m_idActionBackTexture);
			rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, BI_COLOR_VERTEX_FORMAT, 2, m_ActionBackVertex,
									sizeof(BI_COLOR_VERTEX), "battle_rectangle");
		}
		
        // show icon
        if ((m_idIconTexture != -1L) && m_bThatRealAction)
        {
            rs->TextureSet(0, m_idIconTexture);
            rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, BI_ONETEX_VERTEX_FORMAT, 2, m_IconVertex,
                                sizeof(BI_ONETEXTURE_VERTEX), "battle_rectangle");

            m_ActionHint1.Print();
            m_ActionHint2.Print();
        }
    }
	
	// time speed
	if(m_bShowLogStrings && bShowTimeSpeed)
	{
		
		rs->TextureSet(0, m_idTimeSpeedIconTexture);
			rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, BI_COLOR_VERTEX_FORMAT, 2, m_TimeSpeedIconVertex,
							sizeof(BI_COLOR_VERTEX), "battle_tex_col_Rectangle");
							
		rs->ExtPrint(m_TimeSpeedfontID, m_dwTimeSpeedColor, 0, PR_ALIGN_LEFT, true, m_fTimeSpeedFontScale,
                             0, 0, m_nTimeSpeedLeft, m_nTimeSpeedUp, "%s", TimeSpeedText.c_str());
		
		
	}	
	
	//level up
	if (m_bShowLogStrings && AlphaLU > 0.f)
    {
		m_LUBackVertex[0].col = m_dwLUBackColor + (static_cast<int32_t>(AlphaLU) << 24);
		m_LUBackVertex[1].col = m_dwLUBackColor + (static_cast<int32_t>(AlphaLU) << 24);
		m_LUBackVertex[2].col = m_dwLUBackColor + (static_cast<int32_t>(AlphaLU) << 24);
		m_LUBackVertex[3].col = m_dwLUBackColor + (static_cast<int32_t>(AlphaLU) << 24);
		
		rs->TextureSet(0, m_idLUBackTexture);
		rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, BI_COLOR_VERTEX_FORMAT, 2, m_LUBackVertex,
							sizeof(BI_COLOR_VERTEX), "battle_tex_col_Rectangle"); 
							
		rs->ExtPrint(m_LUStringfontID, m_dwLUStringColor + (static_cast<int32_t>(AlphaLU) << 24), 0, PR_ALIGN_CENTER, true, m_fLUStringFontScale,
						 0, 0, m_nLUStringLeft, m_nLUStringUp, "%s", LUStringText.c_str());
		
		rs->ExtPrint(m_LURankfontID, m_dwLURankColor + (static_cast<int32_t>(AlphaLU) << 24), 0, PR_ALIGN_CENTER, true, m_fLURankFontScale,
						 0, 0, m_nLURankLeft, m_nLURankUp, "%s", LURankText.c_str());
    }
	
    // Show log strings
    if (m_bShowLogStrings)
    {
        if (m_sRoot == nullptr && m_pRoot == nullptr)
            return; 
        auto *ptr = m_sRoot;
        int32_t nAlign = PR_ALIGN_LEFT;
        auto strX = m_nWindowLeft;
        if (m_nWindowRight >= 0)
        {
            strX = m_nWindowRight;
            nAlign = PR_ALIGN_RIGHT;
        }
        auto strY = m_nStringBegin;
        while (ptr != nullptr)
        {
            // rs->Print(m_fontID,m_dwColor,strX,strY,"%s",ptr->str);
            if (ptr->alpha <= 255.f)
                // rs->Print(m_fontID,m_dwColor+(int32_t(ptr->alpha)<<24),strX,m_nWindowUp+(int32_t)ptr->offset,"%s",ptr->str);
                rs->ExtPrint(m_fontID, m_dwColor + (static_cast<int32_t>(ptr->alpha) << 24), 0, nAlign, true, m_fFontScale,
                             0, 0, strX, m_nWindowUp + static_cast<int32_t>(ptr->offset), "%s", ptr->str);
            else
                // rs->Print(m_fontID,m_dwColor+0xFF000000,strX,m_nWindowUp+(int32_t)ptr->offset,"%s",ptr->str);
                rs->ExtPrint(m_fontID, m_dwColor + 0xFF000000, 0, nAlign, true, m_fFontScale, 0, 0, strX,
                             m_nWindowUp + static_cast<int32_t>(ptr->offset), "%s", ptr->str);
            strY += m_nStringOffset;
            ptr = ptr->next;
        }
    }
	
	// Show Pic Info strings
    if (m_bShowLogStrings)
    {
        if (m_pRoot == nullptr)
            return;
        auto *ptr = m_pRoot;
        int32_t nPIAlign = PR_ALIGN_LEFT;
        auto strPIX = m_nPicInfoWindowLeft;
        if (m_nPicInfoWindowRight >= 0)
        {
            strPIX = m_nPicInfoWindowRight;
            nPIAlign = PR_ALIGN_RIGHT;
        }
		
		if(m_PicInfoBackVertex[0].col != m_dwPicInfoMinColor)
		{ 
			m_PicInfoBackVertex[0].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI) << 24);
			m_PicInfoBackVertex[1].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI) << 24);
			m_PicInfoBackVertex[2].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI) << 24);
			m_PicInfoBackVertex[3].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI) << 24);
			
			m_PicInfoIconVertex[0].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI) << 24);
			m_PicInfoIconVertex[1].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI) << 24);
			m_PicInfoIconVertex[2].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI) << 24);
			m_PicInfoIconVertex[3].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI) << 24);
	
			rs->TextureSet(0, m_idPicInfoBackTexture);
			rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, BI_COLOR_VERTEX_FORMAT, 2, m_PicInfoBackVertex,
							sizeof(BI_COLOR_VERTEX), "battle_tex_col_Rectangle");
							
			rs->TextureSet(0, m_idPicInfoIconTexture);
			rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, BI_COLOR_VERTEX_FORMAT, 2, m_PicInfoIconVertex,
						sizeof(BI_COLOR_VERTEX), "battle_tex_col_Rectangle"); 
		}
		
		if(m_PicInfoBackVertex1[0].col != m_dwPicInfoMinColor)
		{ 
			m_PicInfoBackVertex1[0].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI2) << 24);
			m_PicInfoBackVertex1[1].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI2) << 24);
			m_PicInfoBackVertex1[2].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI2) << 24);
			m_PicInfoBackVertex1[3].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI2) << 24);
			
			m_PicInfoIconVertex1[0].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI2) << 24);
			m_PicInfoIconVertex1[1].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI2) << 24);
			m_PicInfoIconVertex1[2].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI2) << 24);
			m_PicInfoIconVertex1[3].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI2) << 24); 
	
			rs->TextureSet(0, m_idPicInfoBackTexture);
			rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, BI_COLOR_VERTEX_FORMAT, 2, m_PicInfoBackVertex1,
							sizeof(BI_COLOR_VERTEX), "battle_tex_col_Rectangle");
							
			rs->TextureSet(0, m_idPicInfoIconTexture);
			rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, BI_COLOR_VERTEX_FORMAT, 2, m_PicInfoIconVertex1,
						sizeof(BI_COLOR_VERTEX), "battle_tex_col_Rectangle");  
		}
		
		if(m_PicInfoBackVertex2[0].col != m_dwPicInfoMinColor)
		{ 
			m_PicInfoBackVertex2[0].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI3) << 24);
			m_PicInfoBackVertex2[1].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI3) << 24);
			m_PicInfoBackVertex2[2].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI3) << 24);
			m_PicInfoBackVertex2[3].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI3) << 24);
			
			m_PicInfoIconVertex2[0].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI3) << 24);
			m_PicInfoIconVertex2[1].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI3) << 24);
			m_PicInfoIconVertex2[2].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI3) << 24);
			m_PicInfoIconVertex2[3].col = m_dwPicInfoMinColor + (static_cast<int32_t>(AlphaBI3) << 24); 
	
			rs->TextureSet(0, m_idPicInfoBackTexture);
			rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, BI_COLOR_VERTEX_FORMAT, 2, m_PicInfoBackVertex2,
							sizeof(BI_COLOR_VERTEX), "battle_tex_col_Rectangle");
							
			rs->TextureSet(0, m_idPicInfoIconTexture);
			rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, BI_COLOR_VERTEX_FORMAT, 2, m_PicInfoIconVertex2,
						sizeof(BI_COLOR_VERTEX), "battle_tex_col_Rectangle");  
		}
        auto strPIY = m_nPicInfoStringBegin;	
        while (ptr != nullptr)
        {
            rs->ExtPrint(m_PicInfofontID, m_dwPicInfoColor + (static_cast<int32_t>(ptr->alpha) << 24), 0, nPIAlign, true, m_fPicInfoFontScale,
                             0, 0, strPIX, m_nPicInfoWindowUp + static_cast<int32_t>(ptr->offset), "%s", ptr->str);
            
            strPIY += m_nPicInfoStringOffset;
            ptr = ptr->next;
        }
    }
}

void ILogAndActions::Create(bool bFastComShow, bool bLogStringShow)
{
    m_bShowActiveCommand = bFastComShow;
    m_bShowLogStrings = bLogStringShow;

    // Set parameters for the active action icon
    auto *pA = AttributesPointer->GetAttributeClass("ActiveActions");
    if (pA != nullptr)
    {
        m_idIconTexture = rs->TextureCreate(pA->GetAttribute("TextureName"));
        m_horzDiv = pA->GetAttributeAsDword("horzQ", 1);
        m_vertDiv = pA->GetAttributeAsDword("vertQ", 1);
        m_nIconWidth = pA->GetAttributeAsDword("width", 64);
        m_nIconHeight = pA->GetAttributeAsDword("height", 64);
        m_nIconLeft = pA->GetAttributeAsDword("left", 0);
        m_nIconUp = pA->GetAttributeAsDword("top", 0);

        m_ActionHint1.Init(rs, pA->GetAttributeClass("text1"));
        m_ActionHint2.Init(rs, pA->GetAttributeClass("text2"));
    }
    else
    {
        m_idIconTexture = -1L;
        m_horzDiv = 1;
        m_vertDiv = 1;
        m_nIconWidth = 64;
        m_nIconHeight = 64;
        m_nIconLeft = 0;
        m_nIconUp = 0;
    }
    // build a rectangle for drawing the active action
    m_IconVertex[0].w = m_IconVertex[1].w = m_IconVertex[2].w = m_IconVertex[3].w = .5f;
    m_IconVertex[0].pos.z = m_IconVertex[1].pos.z = m_IconVertex[2].pos.z = m_IconVertex[3].pos.z = 1.f;
    m_IconVertex[0].pos.x = m_IconVertex[1].pos.x = static_cast<float>(m_nIconLeft);
    m_IconVertex[2].pos.x = m_IconVertex[3].pos.x = static_cast<float>(m_nIconLeft + m_nIconWidth);
    m_IconVertex[0].pos.y = m_IconVertex[2].pos.y = static_cast<float>(m_nIconUp);
    m_IconVertex[1].pos.y = m_IconVertex[3].pos.y = static_cast<float>(m_nIconUp + m_nIconHeight);
    m_IconVertex[0].tu = m_IconVertex[1].tu = 0.f;
    m_IconVertex[2].tu = m_IconVertex[3].tu = 1.f / static_cast<float>(m_horzDiv);
    m_IconVertex[0].tv = m_IconVertex[2].tv = 0.f;
    m_IconVertex[1].tv = m_IconVertex[3].tv = 1.f / static_cast<float>(m_vertDiv);

    // set parameters for the previous action lines
    pA = AttributesPointer->GetAttributeClass("Log");
    if (pA != nullptr)
    {
        m_nWindowWidth = pA->GetAttributeAsDword("width", 200);
        m_nWindowHeight = pA->GetAttributeAsDword("height", 128);
        m_nWindowLeft = pA->GetAttributeAsDword("left", -1);
        m_nWindowRight = pA->GetAttributeAsDword("right", -1);
        m_nWindowUp = pA->GetAttributeAsDword("up", 0);
        m_fontID = rs->LoadFont(pA->GetAttribute("font"));
        m_fFontScale = pA->GetAttributeAsFloat("fontscale", 1.f);
        m_dwColor = pA->GetAttributeAsDword("color", 0x00FFFFFF);
        m_nStringOffset = pA->GetAttributeAsDword("offsetString", 24);
        m_fShiftSpeed = pA->GetAttributeAsFloat("speed", 1.f / 50.f);
        m_fBlendSpeed = pA->GetAttributeAsFloat("color_speed", 1.f / 50.f);
    }
    else
    {
        m_nWindowWidth = 200;
        m_nWindowHeight = 128;
        m_nWindowLeft = 64;
        m_nWindowUp = 0;
        m_fontID = -1L;
        m_fFontScale = 1.f;
        m_dwColor = 0x00FFFFFF;
        m_nStringBegin = 0;
        m_nStringOffset = 24;
        m_fShiftSpeed = 1.f / 50.f;
        m_fBlendSpeed = 1.f / 50.f;
    }
	
	//backimage
	pA = AttributesPointer->GetAttributeClass("ActiveActionsBack");
	if (pA != nullptr)
	{
		m_idActionBackTexture = rs->TextureCreate(pA->GetAttribute("TextureName"));
		m_nActionBackWidth = pA->GetAttributeAsDword("width", 64);
		m_nActionBackHeight = pA->GetAttributeAsDword("height", 64);
		m_nActionBackCentr = pA->GetAttributeAsDword("centr", 0);
		m_nActionBackUp = pA->GetAttributeAsDword("top", 0);
		m_dwActionBackColor = pA->GetAttributeAsDword("color", 0x00FFFFFF);
	}
	else
	{
		m_idActionBackTexture = -1L;
		m_nActionBackWidth = 64;
		m_nActionBackHeight = 64;
		m_nActionBackCentr = 0;
		m_nActionBackUp = 0;
		m_dwActionBackColor = 0x00FFFFFF;
	}
	m_ActionBackVertex[0].w = m_ActionBackVertex[1].w = m_ActionBackVertex[2].w = m_ActionBackVertex[3].w = .5f;
	m_ActionBackVertex[0].pos.z = m_ActionBackVertex[1].pos.z = m_ActionBackVertex[2].pos.z = m_ActionBackVertex[3].pos.z = 1.f;
	m_ActionBackVertex[0].col = m_ActionBackVertex[1].col = m_ActionBackVertex[2].col = m_ActionBackVertex[3].col = m_dwActionBackColor;
	m_ActionBackVertex[0].pos.x = static_cast<float>(m_nActionBackCentr);
	m_ActionBackVertex[0].pos.y = static_cast<float>(m_nActionBackUp);
	m_ActionBackVertex[0].tu = 0.f;
	m_ActionBackVertex[0].tv = 0.f;

	m_ActionBackVertex[1].pos.x = static_cast<float>(m_nActionBackCentr);
	m_ActionBackVertex[1].pos.y = static_cast<float>(m_nActionBackUp + m_nActionBackHeight);
	m_ActionBackVertex[1].tu = 0.f;
	m_ActionBackVertex[1].tv = 1.f ;

	m_ActionBackVertex[2].pos.x = static_cast<float>(m_nActionBackCentr + m_nActionBackWidth);
	m_ActionBackVertex[2].pos.y = static_cast<float>(m_nActionBackUp);
	m_ActionBackVertex[2].tu = 1.f;
	m_ActionBackVertex[2].tv = 0.f;

	m_ActionBackVertex[3].pos.x = static_cast<float>(m_nActionBackCentr + m_nActionBackWidth);
	m_ActionBackVertex[3].pos.y = static_cast<float>(m_nActionBackUp + m_nActionBackHeight);
	m_ActionBackVertex[3].tu = 1.f ;
	m_ActionBackVertex[3].tv = 1.f ; 
	
	
	// set param for pic info
	PIstage = 0;
	pA = AttributesPointer->GetAttributeClass("PicInfo");
    if (pA != nullptr)
    {
        m_nPicInfoWindowWidth = pA->GetAttributeAsDword("width", 200);
        m_nPicInfoWindowHeight = pA->GetAttributeAsDword("height", 128);
        m_nPicInfoWindowLeft = pA->GetAttributeAsDword("left", -1);
        m_nPicInfoWindowRight = pA->GetAttributeAsDword("right", -1);
        m_nPicInfoWindowUp = pA->GetAttributeAsDword("up", 0);
        m_PicInfofontID = rs->LoadFont(pA->GetAttribute("font"));
        m_fPicInfoFontScale = pA->GetAttributeAsFloat("fontscale", 1.f);
        m_dwPicInfoColor = pA->GetAttributeAsDword("color", 0x00FFFFFF);
        m_nPicInfoStringOffset = pA->GetAttributeAsDword("offsetString", 24);
        m_fPicInfoShiftSpeed = pA->GetAttributeAsFloat("speed", 1.f / 50.f);
		m_fPicInfoBlendSpeed = pA->GetAttributeAsFloat("color_speed", 1.f / 50.f);
		m_dwPicInfoMaxColor = pA->GetAttributeAsDword("maxcolor", 0x00FFFFFF);
        m_dwPicInfoMinColor = pA->GetAttributeAsDword("mincolor", 0x00FFFFFF);
    }
    else
    {
        m_nPicInfoWindowWidth = 200;
        m_nPicInfoWindowHeight = 128;
        m_nPicInfoWindowLeft = 64;
        m_nPicInfoWindowUp = 0;
        m_PicInfofontID = -1L;
        m_fPicInfoFontScale = 1.f;
        m_dwPicInfoColor = 0x00FFFFFF;
        m_nPicInfoStringBegin = 0;
        m_nPicInfoStringOffset = 24;
        m_fPicInfoShiftSpeed = 1.f / 50.f;
        m_fPicInfoBlendSpeed = 1.f / 50.f;
		m_dwPicInfoMaxColor = 0x00FFFFFF;
        m_dwPicInfoMinColor = 0x00FFFFFF;
    }
	AlphaBI = 0.f;
	AlphaBI2 = 0.f;
	AlphaBI3 = 0.f;
	// backimage
	pA = AttributesPointer->GetAttributeClass("PicInfoBack");
	if (pA != nullptr)
	{
		m_idPicInfoBackTexture = rs->TextureCreate(pA->GetAttribute("TextureName"));
		m_nPicInfoBackWidth = pA->GetAttributeAsDword("width", 64);
		m_nPicInfoBackHeight = pA->GetAttributeAsDword("height", 64);
		m_nPicInfoBackLeft = pA->GetAttributeAsDword("left", 0);
		m_nPicInfoBackUp = pA->GetAttributeAsDword("top", 0);
	}
	else
	{
		m_idPicInfoBackTexture = -1L;
		m_nPicInfoBackWidth = 64;
		m_nPicInfoBackHeight = 64;
		m_nPicInfoBackLeft = 0;
		m_nPicInfoBackUp = 0;
	}
	m_PicInfoBackVertex[0].w = m_PicInfoBackVertex[1].w = m_PicInfoBackVertex[2].w = m_PicInfoBackVertex[3].w = .5f;
	m_PicInfoBackVertex[0].pos.z = m_PicInfoBackVertex[1].pos.z = m_PicInfoBackVertex[2].pos.z = m_PicInfoBackVertex[3].pos.z = 1.f;
	m_PicInfoBackVertex[0].col = m_PicInfoBackVertex[1].col = m_PicInfoBackVertex[2].col = m_PicInfoBackVertex[3].col = m_dwPicInfoMinColor;
	m_PicInfoBackVertex[0].pos.x = static_cast<float>(m_nPicInfoBackLeft);
	m_PicInfoBackVertex[0].pos.y = static_cast<float>(m_nPicInfoBackUp);
	m_PicInfoBackVertex[0].tu = 0.f;
	m_PicInfoBackVertex[0].tv = 0.f;

	m_PicInfoBackVertex[1].pos.x = static_cast<float>(m_nPicInfoBackLeft);
	m_PicInfoBackVertex[1].pos.y = static_cast<float>(m_nPicInfoBackUp + m_nPicInfoBackHeight);
	m_PicInfoBackVertex[1].tu = 0.f;
	m_PicInfoBackVertex[1].tv = 1.f ;

	m_PicInfoBackVertex[2].pos.x = static_cast<float>(m_nPicInfoBackLeft + m_nPicInfoBackWidth);
	m_PicInfoBackVertex[2].pos.y = static_cast<float>(m_nPicInfoBackUp);
	m_PicInfoBackVertex[2].tu = 1.f;
	m_PicInfoBackVertex[2].tv = 0.f;

	m_PicInfoBackVertex[3].pos.x = static_cast<float>(m_nPicInfoBackLeft + m_nPicInfoBackWidth);
	m_PicInfoBackVertex[3].pos.y = static_cast<float>(m_nPicInfoBackUp + m_nPicInfoBackHeight);
	m_PicInfoBackVertex[3].tu = 1.f ;
	m_PicInfoBackVertex[3].tv = 1.f ; 
	
	// Icon 
	pA = AttributesPointer->GetAttributeClass("PicInfoIcon");
	if (pA != nullptr)
	{
		m_idPicInfoIconTexture = rs->TextureCreate(pA->GetAttribute("TextureName"));
		m_nPicInfoIconhorzDiv = pA->GetAttributeAsDword("horzQ", 1);
		m_nPicInfoIconvertDiv = pA->GetAttributeAsDword("vertQ", 1);
		m_nPicInfoIconWidth = pA->GetAttributeAsDword("width", 64);
		m_nPicInfoIconHeight = pA->GetAttributeAsDword("height", 64);
		m_nPicInfoIconLeft = pA->GetAttributeAsDword("left", 0);
		m_nPicInfoIconUp = pA->GetAttributeAsDword("top", 0);
	}
	else
	{
		m_idPicInfoIconTexture = -1L;
		m_nPicInfoIconhorzDiv = 1;
		m_nPicInfoIconvertDiv = 1;
		m_nPicInfoIconWidth = 64;
		m_nPicInfoIconHeight = 64;
		m_nPicInfoIconLeft = 0;
		m_nPicInfoIconUp = 0;
	} 
	
	FRECT texRect;
	CalculateTexturePos(texRect, m_nPicInfoIconhorzDiv, m_nPicInfoIconvertDiv, m_nPicInfoIconIndex);
	
	m_PicInfoIconVertex[0].w = m_PicInfoIconVertex[1].w = m_PicInfoIconVertex[2].w = m_PicInfoIconVertex[3].w = .5f;
	m_PicInfoIconVertex[0].pos.z = m_PicInfoIconVertex[1].pos.z = m_PicInfoIconVertex[2].pos.z = m_PicInfoIconVertex[3].pos.z = 1.f;
	m_PicInfoIconVertex[0].col = m_PicInfoIconVertex[1].col = m_PicInfoIconVertex[2].col = m_PicInfoIconVertex[3].col = m_dwPicInfoMinColor;
	m_PicInfoIconVertex[0].pos.x = static_cast<float>(m_nPicInfoIconLeft);
	m_PicInfoIconVertex[0].pos.y = static_cast<float>(m_nPicInfoIconUp);
	m_PicInfoIconVertex[0].tu = texRect.left;
	m_PicInfoIconVertex[0].tv = texRect.top;

	m_PicInfoIconVertex[1].pos.x = static_cast<float>(m_nPicInfoIconLeft);
	m_PicInfoIconVertex[1].pos.y = static_cast<float>(m_nPicInfoIconUp + m_nPicInfoIconHeight);
	m_PicInfoIconVertex[1].tu = texRect.left;
	m_PicInfoIconVertex[1].tv = texRect.bottom;

	m_PicInfoIconVertex[2].pos.x = static_cast<float>(m_nPicInfoIconLeft + m_nPicInfoIconWidth);
	m_PicInfoIconVertex[2].pos.y = static_cast<float>(m_nPicInfoIconUp);
	m_PicInfoIconVertex[2].tu = texRect.right;
	m_PicInfoIconVertex[2].tv = texRect.top;

	m_PicInfoIconVertex[3].pos.x = static_cast<float>(m_nPicInfoIconLeft + m_nPicInfoIconWidth);
	m_PicInfoIconVertex[3].pos.y = static_cast<float>(m_nPicInfoIconUp + m_nPicInfoIconHeight);
	m_PicInfoIconVertex[3].tu = texRect.right;
	m_PicInfoIconVertex[3].tv = texRect.bottom;
	
	//second line
	m_PicInfoBackVertex1[0].w = m_PicInfoBackVertex1[1].w = m_PicInfoBackVertex1[2].w = m_PicInfoBackVertex1[3].w = .5f;
	m_PicInfoBackVertex1[0].pos.z = m_PicInfoBackVertex1[1].pos.z = m_PicInfoBackVertex1[2].pos.z = m_PicInfoBackVertex1[3].pos.z = 1.f;
	m_PicInfoBackVertex1[0].col = m_PicInfoBackVertex[1].col = m_PicInfoBackVertex1[2].col = m_PicInfoBackVertex1[3].col = m_dwPicInfoMinColor;
	m_PicInfoBackVertex1[0].pos.x = static_cast<float>(m_nPicInfoBackLeft);
	m_PicInfoBackVertex1[0].pos.y = static_cast<float>(m_nPicInfoBackUp) + static_cast<float>(m_nPicInfoStringOffset);
	m_PicInfoBackVertex1[0].tu = 0.f;
	m_PicInfoBackVertex1[0].tv = 0.f;

	m_PicInfoBackVertex1[1].pos.x = static_cast<float>(m_nPicInfoBackLeft);
	m_PicInfoBackVertex1[1].pos.y = static_cast<float>(m_nPicInfoBackUp + m_nPicInfoBackHeight) + static_cast<float>(m_nPicInfoStringOffset);
	m_PicInfoBackVertex1[1].tu = 0.f;
	m_PicInfoBackVertex1[1].tv = 1.f ;

	m_PicInfoBackVertex1[2].pos.x = static_cast<float>(m_nPicInfoBackLeft + m_nPicInfoBackWidth);
	m_PicInfoBackVertex1[2].pos.y = static_cast<float>(m_nPicInfoBackUp) + static_cast<float>(m_nPicInfoStringOffset);
	m_PicInfoBackVertex1[2].tu = 1.f;
	m_PicInfoBackVertex1[2].tv = 0.f;

	m_PicInfoBackVertex1[3].pos.x = static_cast<float>(m_nPicInfoBackLeft + m_nPicInfoBackWidth);
	m_PicInfoBackVertex1[3].pos.y = static_cast<float>(m_nPicInfoBackUp + m_nPicInfoBackHeight) + static_cast<float>(m_nPicInfoStringOffset);
	m_PicInfoBackVertex1[3].tu = 1.f ;
	m_PicInfoBackVertex1[3].tv = 1.f ; 
	
	
	CalculateTexturePos(texRect, m_nPicInfoIconhorzDiv, m_nPicInfoIconvertDiv, m_nPicInfoIconIndex);
	
	m_PicInfoIconVertex1[0].w = m_PicInfoIconVertex1[1].w = m_PicInfoIconVertex1[2].w = m_PicInfoIconVertex1[3].w = .5f;
	m_PicInfoIconVertex1[0].pos.z = m_PicInfoIconVertex1[1].pos.z = m_PicInfoIconVertex1[2].pos.z = m_PicInfoIconVertex1[3].pos.z = 1.f;
	m_PicInfoIconVertex1[0].col = m_PicInfoIconVertex1[1].col = m_PicInfoIconVertex1[2].col = m_PicInfoIconVertex1[3].col = m_dwPicInfoMinColor;
	m_PicInfoIconVertex1[0].pos.x = static_cast<float>(m_nPicInfoIconLeft);
	m_PicInfoIconVertex1[0].pos.y = static_cast<float>(m_nPicInfoIconUp) + static_cast<float>(m_nPicInfoStringOffset);
	m_PicInfoIconVertex1[0].tu = texRect.left;
	m_PicInfoIconVertex1[0].tv = texRect.top;

	m_PicInfoIconVertex1[1].pos.x = static_cast<float>(m_nPicInfoIconLeft);
	m_PicInfoIconVertex1[1].pos.y = static_cast<float>(m_nPicInfoIconUp + m_nPicInfoIconHeight) + static_cast<float>(m_nPicInfoStringOffset);
	m_PicInfoIconVertex1[1].tu = texRect.left;
	m_PicInfoIconVertex1[1].tv = texRect.bottom;

	m_PicInfoIconVertex1[2].pos.x = static_cast<float>(m_nPicInfoIconLeft + m_nPicInfoIconWidth);
	m_PicInfoIconVertex1[2].pos.y = static_cast<float>(m_nPicInfoIconUp) + static_cast<float>(m_nPicInfoStringOffset);
	m_PicInfoIconVertex1[2].tu = texRect.right;
	m_PicInfoIconVertex1[2].tv = texRect.top;

	m_PicInfoIconVertex1[3].pos.x = static_cast<float>(m_nPicInfoIconLeft + m_nPicInfoIconWidth);
	m_PicInfoIconVertex1[3].pos.y = static_cast<float>(m_nPicInfoIconUp + m_nPicInfoIconHeight) + static_cast<float>(m_nPicInfoStringOffset);
	m_PicInfoIconVertex1[3].tu = texRect.right;
	m_PicInfoIconVertex1[3].tv = texRect.bottom;
	
	
	//3d line
	m_PicInfoBackVertex2[0].w = m_PicInfoBackVertex2[1].w = m_PicInfoBackVertex2[2].w = m_PicInfoBackVertex2[3].w = .5f;
	m_PicInfoBackVertex2[0].pos.z = m_PicInfoBackVertex2[1].pos.z = m_PicInfoBackVertex2[2].pos.z = m_PicInfoBackVertex2[3].pos.z = 1.f;
	m_PicInfoBackVertex2[0].col = m_PicInfoBackVertex[1].col = m_PicInfoBackVertex2[2].col = m_PicInfoBackVertex2[3].col = m_dwPicInfoMinColor;
	m_PicInfoBackVertex2[0].pos.x = static_cast<float>(m_nPicInfoBackLeft);
	m_PicInfoBackVertex2[0].pos.y = static_cast<float>(m_nPicInfoBackUp) + static_cast<float>(m_nPicInfoStringOffset*2);
	m_PicInfoBackVertex2[0].tu = 0.f;
	m_PicInfoBackVertex2[0].tv = 0.f;

	m_PicInfoBackVertex2[1].pos.x = static_cast<float>(m_nPicInfoBackLeft);
	m_PicInfoBackVertex2[1].pos.y = static_cast<float>(m_nPicInfoBackUp + m_nPicInfoBackHeight) + static_cast<float>(m_nPicInfoStringOffset*2);
	m_PicInfoBackVertex2[1].tu = 0.f;
	m_PicInfoBackVertex2[1].tv = 1.f ;

	m_PicInfoBackVertex2[2].pos.x = static_cast<float>(m_nPicInfoBackLeft + m_nPicInfoBackWidth);
	m_PicInfoBackVertex2[2].pos.y = static_cast<float>(m_nPicInfoBackUp) + static_cast<float>(m_nPicInfoStringOffset*2);
	m_PicInfoBackVertex2[2].tu = 1.f;
	m_PicInfoBackVertex2[2].tv = 0.f;

	m_PicInfoBackVertex2[3].pos.x = static_cast<float>(m_nPicInfoBackLeft + m_nPicInfoBackWidth);
	m_PicInfoBackVertex2[3].pos.y = static_cast<float>(m_nPicInfoBackUp + m_nPicInfoBackHeight) + static_cast<float>(m_nPicInfoStringOffset*2);
	m_PicInfoBackVertex2[3].tu = 1.f ;
	m_PicInfoBackVertex2[3].tv = 1.f ; 
	
	
	CalculateTexturePos(texRect, m_nPicInfoIconhorzDiv, m_nPicInfoIconvertDiv, m_nPicInfoIconIndex);
	
	m_PicInfoIconVertex2[0].w = m_PicInfoIconVertex2[1].w = m_PicInfoIconVertex2[2].w = m_PicInfoIconVertex2[3].w = .5f;
	m_PicInfoIconVertex2[0].pos.z = m_PicInfoIconVertex2[1].pos.z = m_PicInfoIconVertex2[2].pos.z = m_PicInfoIconVertex2[3].pos.z = 1.f;
	m_PicInfoIconVertex2[0].col = m_PicInfoIconVertex2[1].col = m_PicInfoIconVertex2[2].col = m_PicInfoIconVertex2[3].col = m_dwPicInfoMinColor;
	m_PicInfoIconVertex2[0].pos.x = static_cast<float>(m_nPicInfoIconLeft);
	m_PicInfoIconVertex2[0].pos.y = static_cast<float>(m_nPicInfoIconUp) + static_cast<float>(m_nPicInfoStringOffset*2);
	m_PicInfoIconVertex2[0].tu = texRect.left;
	m_PicInfoIconVertex2[0].tv = texRect.top;

	m_PicInfoIconVertex2[1].pos.x = static_cast<float>(m_nPicInfoIconLeft);
	m_PicInfoIconVertex2[1].pos.y = static_cast<float>(m_nPicInfoIconUp + m_nPicInfoIconHeight) + static_cast<float>(m_nPicInfoStringOffset*2);
	m_PicInfoIconVertex2[1].tu = texRect.left;
	m_PicInfoIconVertex2[1].tv = texRect.bottom;

	m_PicInfoIconVertex2[2].pos.x = static_cast<float>(m_nPicInfoIconLeft + m_nPicInfoIconWidth);
	m_PicInfoIconVertex2[2].pos.y = static_cast<float>(m_nPicInfoIconUp) + static_cast<float>(m_nPicInfoStringOffset*2);
	m_PicInfoIconVertex2[2].tu = texRect.right;
	m_PicInfoIconVertex2[2].tv = texRect.top;

	m_PicInfoIconVertex2[3].pos.x = static_cast<float>(m_nPicInfoIconLeft + m_nPicInfoIconWidth);
	m_PicInfoIconVertex2[3].pos.y = static_cast<float>(m_nPicInfoIconUp + m_nPicInfoIconHeight) + static_cast<float>(m_nPicInfoStringOffset*2);
	m_PicInfoIconVertex2[3].tu = texRect.right;
	m_PicInfoIconVertex2[3].tv = texRect.bottom;
	
	
	// Time Speed
	// text
	pA = AttributesPointer->GetAttributeClass("timespeedtext");
    if (pA != nullptr)
    {
        m_nTimeSpeedLeft = pA->GetAttributeAsDword("left", -1);
        m_nTimeSpeedUp = pA->GetAttributeAsDword("up", 0);
        m_TimeSpeedfontID = rs->LoadFont(pA->GetAttribute("font"));
        m_fTimeSpeedFontScale = pA->GetAttributeAsFloat("fontscale", 1.f);
        m_dwTimeSpeedColor = pA->GetAttributeAsDword("color", 0x00FFFFFF);
    }
    else
    {
        
        m_nTimeSpeedLeft = 64;
        m_nTimeSpeedUp = 0;
        m_TimeSpeedfontID = -1L;
        m_fTimeSpeedFontScale = 1.f;
        m_dwTimeSpeedColor = 0x00FFFFFF;
    }
	
	// icon
	pA = AttributesPointer->GetAttributeClass("timespeedicon");
	if (pA != nullptr)
	{
		m_idTimeSpeedIconTexture = rs->TextureCreate(pA->GetAttribute("texturename"));
		m_nTimeSpeedIconWidth = pA->GetAttributeAsDword("width", 64);
		m_nTimeSpeedIconHeight = pA->GetAttributeAsDword("height", 64);
		m_nTimeSpeedIconLeft = pA->GetAttributeAsDword("left", 0);
		m_nTimeSpeedIconUp = pA->GetAttributeAsDword("top", 0);
		m_dwTimeSpeedIconColor = pA->GetAttributeAsDword("color", 0x00FFFFFF);
	}
	else
	{
		m_idTimeSpeedIconTexture = -1L;
		m_nTimeSpeedIconWidth = 64;
		m_nTimeSpeedIconHeight = 64;
		m_nTimeSpeedIconLeft = 0;
		m_nTimeSpeedIconUp = 0;
		m_dwTimeSpeedIconColor = 0x00FFFFFF;
	}  
	
	m_TimeSpeedIconVertex[0].w = m_TimeSpeedIconVertex[1].w = m_TimeSpeedIconVertex[2].w = m_TimeSpeedIconVertex[3].w = .5f;
	m_TimeSpeedIconVertex[0].pos.z = m_TimeSpeedIconVertex[1].pos.z = m_TimeSpeedIconVertex[2].pos.z = m_TimeSpeedIconVertex[3].pos.z = 1.f;
	m_TimeSpeedIconVertex[0].col = m_TimeSpeedIconVertex[1].col = m_TimeSpeedIconVertex[2].col = m_TimeSpeedIconVertex[3].col = m_dwTimeSpeedIconColor;
	m_TimeSpeedIconVertex[0].pos.x = static_cast<float>(m_nTimeSpeedIconLeft);
	m_TimeSpeedIconVertex[0].pos.y = static_cast<float>(m_nTimeSpeedIconUp);
	m_TimeSpeedIconVertex[0].tu = 0.f;
	m_TimeSpeedIconVertex[0].tv = 0.f;

	m_TimeSpeedIconVertex[1].pos.x = static_cast<float>(m_nTimeSpeedIconLeft);
	m_TimeSpeedIconVertex[1].pos.y = static_cast<float>(m_nTimeSpeedIconUp + m_nTimeSpeedIconHeight);
	m_TimeSpeedIconVertex[1].tu = 0.f;
	m_TimeSpeedIconVertex[1].tv = 1.f ;

	m_TimeSpeedIconVertex[2].pos.x = static_cast<float>(m_nTimeSpeedIconLeft + m_nTimeSpeedIconWidth);
	m_TimeSpeedIconVertex[2].pos.y = static_cast<float>(m_nTimeSpeedIconUp);
	m_TimeSpeedIconVertex[2].tu = 1.f;
	m_TimeSpeedIconVertex[2].tv = 0.f;

	m_TimeSpeedIconVertex[3].pos.x = static_cast<float>(m_nTimeSpeedIconLeft + m_nTimeSpeedIconWidth);
	m_TimeSpeedIconVertex[3].pos.y = static_cast<float>(m_nTimeSpeedIconUp + m_nTimeSpeedIconHeight);
	m_TimeSpeedIconVertex[3].tu = 1.f ;
	m_TimeSpeedIconVertex[3].tv = 1.f ;  
	
	//level up
	//backimage
	pA = AttributesPointer->GetAttributeClass("levelupback");
	if (pA != nullptr)
	{
		m_idLUBackTexture = rs->TextureCreate(pA->GetAttribute("texturename"));
		m_nLUBackWidth = pA->GetAttributeAsDword("width", 64);
		m_nLUBackHeight = pA->GetAttributeAsDword("height", 64);
		m_nLUBackLeft = pA->GetAttributeAsDword("left", 0);
		m_nLUBackUp = pA->GetAttributeAsDword("top", 0);
		m_dwLUBackColor = pA->GetAttributeAsDword("color", 0x00FFFFFF);
	}
	else
	{
		m_idLUBackTexture = -1L;
		m_nLUBackWidth = 64;
		m_nLUBackHeight = 64;
		m_nLUBackLeft = 0;
		m_nLUBackUp = 0;
		m_dwLUBackColor = 0x00FFFFFF;
	}  
	
	m_LUBackVertex[0].w = m_LUBackVertex[1].w = m_LUBackVertex[2].w = m_LUBackVertex[3].w = .5f;
	m_LUBackVertex[0].pos.z = m_LUBackVertex[1].pos.z = m_LUBackVertex[2].pos.z = m_LUBackVertex[3].pos.z = 1.f;
	m_LUBackVertex[0].col = m_LUBackVertex[1].col = m_LUBackVertex[2].col = m_LUBackVertex[3].col = m_dwLUBackColor;
	m_LUBackVertex[0].pos.x = static_cast<float>(m_nLUBackLeft);
	m_LUBackVertex[0].pos.y = static_cast<float>(m_nLUBackUp);
	m_LUBackVertex[0].tu = 0.f;
	m_LUBackVertex[0].tv = 0.f;
	
	m_LUBackVertex[1].pos.x = static_cast<float>(m_nLUBackLeft);
	m_LUBackVertex[1].pos.y = static_cast<float>(m_nLUBackUp + m_nLUBackHeight);
	m_LUBackVertex[1].tu = 0.f;
	m_LUBackVertex[1].tv = 1.f ;

	m_LUBackVertex[2].pos.x = static_cast<float>(m_nLUBackLeft + m_nLUBackWidth);
	m_LUBackVertex[2].pos.y = static_cast<float>(m_nLUBackUp);
	m_LUBackVertex[2].tu = 1.f;
	m_LUBackVertex[2].tv = 0.f;

	m_LUBackVertex[3].pos.x = static_cast<float>(m_nLUBackLeft + m_nLUBackWidth);
	m_LUBackVertex[3].pos.y = static_cast<float>(m_nLUBackUp + m_nLUBackHeight);
	m_LUBackVertex[3].tu = 1.f ;
	m_LUBackVertex[3].tv = 1.f ;
	
	//string
	pA = AttributesPointer->GetAttributeClass("levelupstring");
    if (pA != nullptr)
    {
        m_nLUStringLeft = pA->GetAttributeAsDword("left", -1);
		m_fLUStringSpeed = pA->GetAttributeAsFloat("color_speed", 1.f / 50.f);
        m_nLUStringUp = pA->GetAttributeAsDword("up", 0);
        m_LUStringfontID = rs->LoadFont(pA->GetAttribute("font"));
        m_fLUStringFontScale = pA->GetAttributeAsFloat("fontscale", 1.f);
        m_dwLUStringColor = pA->GetAttributeAsDword("color", 0x00FFFFFF);
    }
    else
    {
        
        m_nLUStringLeft = 64;
        m_nLUStringUp = 0;
        m_LUStringfontID = -1L;
        m_fLUStringFontScale = 1.f;
        m_dwLUStringColor = 0x00FFFFFF;
		m_fLUStringSpeed = 1.f / 50.f;
    }
	
	//rank
	pA = AttributesPointer->GetAttributeClass("leveluprank");
    if (pA != nullptr)
    {
        m_nLURankLeft = pA->GetAttributeAsDword("left", -1);
        m_nLURankUp = pA->GetAttributeAsDword("up", 0);
        m_LURankfontID = rs->LoadFont(pA->GetAttribute("font"));
        m_fLURankFontScale = pA->GetAttributeAsFloat("fontscale", 1.f);
        m_dwLURankColor = pA->GetAttributeAsDword("color", 0x00FFFFFF);
    }
    else
    {
        
        m_nLURankLeft = 64;
        m_nLURankUp = 0;
        m_LURankfontID = -1L;
        m_fLURankFontScale = 1.f;
        m_dwLURankColor = 0x00FFFFFF;
    }
	
	AlphaLU = 0.f;
}

void ILogAndActions::ActionChange(bool bFastComShow, bool bLogStringShow)
{
    m_bShowActiveCommand = bFastComShow;
    m_bShowLogStrings = bLogStringShow;

    m_bThatRealAction = false;

    // Delete the old parameters
    TEXTURE_RELEASE(rs, m_idIconTexture);

    // Set parameters for the active action icon
    ATTRIBUTES *pA = AttributesPointer->GetAttributeClass("ActiveActions");
    if (pA != nullptr)
    {
        m_idIconTexture = rs->TextureCreate(pA->GetAttribute("TextureName"));
        m_horzDiv = pA->GetAttributeAsDword("horzQ", 1);
        m_vertDiv = pA->GetAttributeAsDword("vertQ", 1);
        m_nIconWidth = pA->GetAttributeAsDword("width", 64);
        m_nIconHeight = pA->GetAttributeAsDword("height", 64);
        m_nIconLeft = pA->GetAttributeAsDword("left", 0);
        m_nIconUp = pA->GetAttributeAsDword("top", 0);

        m_ActionHint1.Init(rs, pA->GetAttributeClass("text1"));
        m_ActionHint2.Init(rs, pA->GetAttributeClass("text2"));
    }
    else
    {
        m_idIconTexture = -1L;
        m_horzDiv = 1;
        m_vertDiv = 1;
        m_nIconWidth = 64;
        m_nIconHeight = 64;
        m_nIconLeft = 0;
        m_nIconUp = 0;
    }
    // build a rectangle for drawing the active action
    m_IconVertex[0].w = m_IconVertex[1].w = m_IconVertex[2].w = m_IconVertex[3].w = .5f;
    m_IconVertex[0].pos.z = m_IconVertex[1].pos.z = m_IconVertex[2].pos.z = m_IconVertex[3].pos.z = 1.f;
    m_IconVertex[0].pos.x = m_IconVertex[1].pos.x = static_cast<float>(m_nIconLeft);
    m_IconVertex[2].pos.x = m_IconVertex[3].pos.x = static_cast<float>(m_nIconLeft + m_nIconWidth);
    m_IconVertex[0].pos.y = m_IconVertex[2].pos.y = static_cast<float>(m_nIconUp);
    m_IconVertex[1].pos.y = m_IconVertex[3].pos.y = static_cast<float>(m_nIconUp + m_nIconHeight);
    m_IconVertex[0].tu = m_IconVertex[1].tu = 0.f;
    m_IconVertex[2].tu = m_IconVertex[3].tu = 1.f / static_cast<float>(m_horzDiv);
    m_IconVertex[0].tv = m_IconVertex[2].tv = 0.f;
    m_IconVertex[1].tv = m_IconVertex[3].tv = 1.f / static_cast<float>(m_vertDiv);
	
	//backimage
	pA = AttributesPointer->GetAttributeClass("ActiveActionsBack");
	if (pA != nullptr)
	{
		m_idActionBackTexture = rs->TextureCreate(pA->GetAttribute("TextureName"));
		m_nActionBackWidth = pA->GetAttributeAsDword("width", 64);
		m_nActionBackHeight = pA->GetAttributeAsDword("height", 64);
		m_nActionBackCentr = pA->GetAttributeAsDword("centr", 0);
		m_nActionBackUp = pA->GetAttributeAsDword("top", 0);
		m_dwActionBackColor = pA->GetAttributeAsDword("color", 0x00FFFFFF);
	}
	else
	{
		m_idActionBackTexture = -1L;
		m_nActionBackWidth = 64;
		m_nActionBackHeight = 64;
		m_nActionBackCentr = 0;
		m_nActionBackUp = 0;
		m_dwActionBackColor = 0x00FFFFFF;
	}
	
	m_ActionBackVertex[0].w = m_ActionBackVertex[1].w = m_ActionBackVertex[2].w = m_ActionBackVertex[3].w = .5f;
	m_ActionBackVertex[0].pos.z = m_ActionBackVertex[1].pos.z = m_ActionBackVertex[2].pos.z = m_ActionBackVertex[3].pos.z = 1.f;
	m_ActionBackVertex[0].col = m_ActionBackVertex[1].col = m_ActionBackVertex[2].col = m_ActionBackVertex[3].col = m_dwActionBackColor;
	m_ActionBackVertex[0].pos.x = static_cast<float>(m_nActionBackCentr);
	m_ActionBackVertex[0].pos.y = static_cast<float>(m_nActionBackUp);
	m_ActionBackVertex[0].tu = 0.f;
	m_ActionBackVertex[0].tv = 0.f;

	m_ActionBackVertex[1].pos.x = static_cast<float>(m_nActionBackCentr);
	m_ActionBackVertex[1].pos.y = static_cast<float>(m_nActionBackUp + m_nActionBackHeight);
	m_ActionBackVertex[1].tu = 0.f;
	m_ActionBackVertex[1].tv = 1.f ;

	m_ActionBackVertex[2].pos.x = static_cast<float>(m_nActionBackCentr + m_nActionBackWidth);
	m_ActionBackVertex[2].pos.y = static_cast<float>(m_nActionBackUp);
	m_ActionBackVertex[2].tu = 1.f;
	m_ActionBackVertex[2].tv = 0.f;

	m_ActionBackVertex[3].pos.x = static_cast<float>(m_nActionBackCentr + m_nActionBackWidth);
	m_ActionBackVertex[3].pos.y = static_cast<float>(m_nActionBackUp + m_nActionBackHeight);
	m_ActionBackVertex[3].tu = 1.f ;
	m_ActionBackVertex[3].tv = 1.f ; 
	
	const int32_t nOffset = rs->StringWidth(m_ActionHint2.sText.c_str(), m_ActionHint2.nFont, m_ActionHint2.fScale, 0);
	m_ActionBackVertex[0].pos.x = static_cast<float>(m_nActionBackCentr) - static_cast<float>(nOffset/2) - static_cast<float>(m_nActionBackWidth);

	m_ActionBackVertex[1].pos.x = static_cast<float>(m_nActionBackCentr) - static_cast<float>(nOffset/2) - static_cast<float>(m_nActionBackWidth);

	m_ActionBackVertex[2].pos.x = static_cast<float>(m_nActionBackCentr) + static_cast<float>(nOffset/2) + static_cast<float>(m_nActionBackWidth);

	m_ActionBackVertex[3].pos.x = static_cast<float>(m_nActionBackCentr) + static_cast<float>(nOffset/2) + static_cast<float>(m_nActionBackWidth);
}

void ILogAndActions::Release()
{
    TEXTURE_RELEASE(rs, m_idIconTexture);
    TEXTURE_RELEASE(rs, m_idPicInfoBackTexture);
    TEXTURE_RELEASE(rs, m_idPicInfoIconTexture);
    TEXTURE_RELEASE(rs, m_idTimeSpeedIconTexture);
    TEXTURE_RELEASE(rs, m_idLUBackTexture);
    TEXTURE_RELEASE(rs, m_idActionBackTexture);
	

    rs->UnloadFont(m_fontID);
    while (m_sRoot != nullptr)
    {
        STRING_DESCR *p = m_sRoot;
        m_sRoot = p->next;
        STORM_DELETE(p->str);
        delete p;
    }
	
	rs->UnloadFont(m_PicInfofontID);
    while (m_pRoot != nullptr)
    {
        STRING_DESCR *pp = m_pRoot;
        m_pRoot = pp->next;
        STORM_DELETE(pp->str);
        delete pp;
    }
	
	rs->UnloadFont(m_TimeSpeedfontID);

    m_ActionHint1.Release();
    m_ActionHint2.Release();
    rs = nullptr;
	
	PIstage = 0;
	
	bShowTimeSpeed = false;
}

void ILogAndActions::SetString(const char *str, bool immortal)
{
    if (str == nullptr)
        return;

    // find the last element of the list
    STRING_DESCR *last = m_sRoot;
    if (last != nullptr)
        while (last->next != nullptr)
            last = last->next;

    // Return if such a line already exists and it is last
    if (last != nullptr && last->str != nullptr && storm::iEquals(last->str, str))
        return;

    // create a new line descriptor
    auto *newDescr = new STRING_DESCR;
    if (newDescr == nullptr)
    {
        throw std::runtime_error("Allocate memory error");
    }
    // it will be the last on the list
    newDescr->next = nullptr;
    // add the specified string to it
    const auto len = strlen(str) + 1;
    if ((newDescr->str = new char[len]) == nullptr)
    {
        throw std::runtime_error("Allocate memory error");
    }
    strcpy_s(newDescr->str, len, str);
    // set the maximum visibility
	if (immortal)
        newDescr->alpha = 10000.f;
    else
        newDescr->alpha = 255.f;

    // if the list is empty, put the string as the root
    if (last == nullptr)
    {
        newDescr->offset = static_cast<float>(m_nStringBegin);
        m_sRoot = newDescr;
    }
    // otherwise add it to the end of the list
    else
    {
        newDescr->offset = last->offset + m_nStringOffset;
        last->next = newDescr;
        if (newDescr->offset + m_nStringOffset > m_nWindowHeight)
        {
            const int32_t offsetDelta = static_cast<int32_t>(newDescr->offset) + m_nStringOffset - m_nWindowHeight;
            for (STRING_DESCR *tmpDescr = m_sRoot; tmpDescr != nullptr;)
            {
                if ((tmpDescr->offset -= offsetDelta) < 0)
                {
                    m_sRoot = tmpDescr->next;
                    STORM_DELETE(tmpDescr->str);
                    delete tmpDescr;
                    tmpDescr = m_sRoot;
                    continue;
                }
                tmpDescr = tmpDescr->next;
            }
        }
    }
}

void ILogAndActions::SetPicInfo(const char *str)
{
    if (str == nullptr)
        return;

    // find the last element of the list
    STRING_DESCR *last = m_pRoot;
    if (last != nullptr)
        while (last->next != nullptr)
            last = last->next;

    // Return if such a line already exists and it is last
    if (last != nullptr && last->str != nullptr && storm::iEquals(last->str, str))
        return;
	
	PIstage++;
	FRECT texRect;
	if(PIstage == 1)
	{
		if(AlphaBI < 0.1f)
		{
			AlphaBI = 255.f;
			m_PicInfoBackVertex[0].col = m_PicInfoBackVertex[1].col = m_PicInfoBackVertex[2].col = m_PicInfoBackVertex[3].col = m_dwPicInfoMaxColor;
			m_PicInfoIconVertex[0].col = m_PicInfoIconVertex[1].col = m_PicInfoIconVertex[2].col = m_PicInfoIconVertex[3].col = m_dwPicInfoMaxColor;
			
			
			CalculateTexturePos(texRect, m_nPicInfoIconhorzDiv, m_nPicInfoIconvertDiv, m_nPicInfoIconIndex);
			
			m_PicInfoIconVertex[0].tu = texRect.left;
			m_PicInfoIconVertex[0].tv = texRect.top;

			m_PicInfoIconVertex[1].tu = texRect.left;
			m_PicInfoIconVertex[1].tv = texRect.bottom;

			m_PicInfoIconVertex[2].tu = texRect.right;
			m_PicInfoIconVertex[2].tv = texRect.top;

			m_PicInfoIconVertex[3].tu = texRect.right;
			m_PicInfoIconVertex[3].tv = texRect.bottom;
		}
	}
	
	if(PIstage == 2)
	{
		if(AlphaBI2 < 0.1f)
		{
			AlphaBI2 = 255.f;
			m_PicInfoBackVertex1[0].col = m_PicInfoBackVertex1[1].col = m_PicInfoBackVertex1[2].col = m_PicInfoBackVertex1[3].col = m_dwPicInfoMaxColor;
			m_PicInfoIconVertex1[0].col = m_PicInfoIconVertex1[1].col = m_PicInfoIconVertex1[2].col = m_PicInfoIconVertex1[3].col = m_dwPicInfoMaxColor;
			
			CalculateTexturePos(texRect, m_nPicInfoIconhorzDiv, m_nPicInfoIconvertDiv, m_nPicInfoIconIndex);
			
			m_PicInfoIconVertex1[0].tu = texRect.left;
			m_PicInfoIconVertex1[0].tv = texRect.top;

			m_PicInfoIconVertex1[1].tu = texRect.left;
			m_PicInfoIconVertex1[1].tv = texRect.bottom;

			m_PicInfoIconVertex1[2].tu = texRect.right;
			m_PicInfoIconVertex1[2].tv = texRect.top;

			m_PicInfoIconVertex1[3].tu = texRect.right;
			m_PicInfoIconVertex1[3].tv = texRect.bottom; 
		}
	}
	
	if(PIstage == 3)
	{
		if(AlphaBI3 < 0.1f)
		{
			AlphaBI3 = 255.f;
			m_PicInfoBackVertex2[0].col = m_PicInfoBackVertex2[1].col = m_PicInfoBackVertex2[2].col = m_PicInfoBackVertex2[3].col = m_dwPicInfoMaxColor;
			m_PicInfoIconVertex2[0].col = m_PicInfoIconVertex2[1].col = m_PicInfoIconVertex2[2].col = m_PicInfoIconVertex2[3].col = m_dwPicInfoMaxColor;
			
			CalculateTexturePos(texRect, m_nPicInfoIconhorzDiv, m_nPicInfoIconvertDiv, m_nPicInfoIconIndex);
			
			m_PicInfoIconVertex2[0].tu = texRect.left;
			m_PicInfoIconVertex2[0].tv = texRect.top;

			m_PicInfoIconVertex2[1].tu = texRect.left;
			m_PicInfoIconVertex2[1].tv = texRect.bottom;

			m_PicInfoIconVertex2[2].tu = texRect.right;
			m_PicInfoIconVertex2[2].tv = texRect.top;

			m_PicInfoIconVertex2[3].tu = texRect.right;
			m_PicInfoIconVertex2[3].tv = texRect.bottom; 
		}
	}
    // create a new line descriptor
    auto *newDescr = new STRING_DESCR;
    if (newDescr == nullptr)
    {
        throw std::runtime_error("Allocate memory error");
    }
    // it will be the last on the list
    newDescr->next = nullptr;
    // add the specified string to it
    const auto len = strlen(str) + 1;
    if ((newDescr->str = new char[len]) == nullptr)
    {
        throw std::runtime_error("Allocate memory error");
    }
    strcpy_s(newDescr->str, len, str);
    // set the maximum visibility
    newDescr->alpha = 255.f;

    // if the list is empty, put the string as the root
    if (last == nullptr)
    {
        newDescr->offset = static_cast<float>(m_nPicInfoStringBegin);
        m_pRoot = newDescr;
    }
    // otherwise add it to the end of the list
    else
    {
        newDescr->offset = last->offset + m_nPicInfoStringOffset;
        last->next = newDescr;
        if (newDescr->offset + m_nPicInfoStringOffset > m_nPicInfoWindowHeight)
        {
            const int32_t offsetDelta = static_cast<int32_t>(newDescr->offset) + m_nPicInfoStringOffset - m_nPicInfoWindowHeight;
            for (STRING_DESCR *tmpDescr = m_pRoot; tmpDescr != nullptr;)
            {
                if ((tmpDescr->offset -= offsetDelta) < 0)
                {
                    m_pRoot = tmpDescr->next;
                    STORM_DELETE(tmpDescr->str);
                    delete tmpDescr;
                    tmpDescr = m_pRoot;
                    continue;
                }
                tmpDescr = tmpDescr->next;
            }
        }
    }
		
}

void ILogAndActions::SetLevelUp()
{
	AlphaLU = 255.f;
}

void ILogAndActions::SetAction(const char *actionName)
{
    ATTRIBUTES *pA;

    if (actionName == nullptr)
        return;
    if ((strlen(actionName) + 1) > sizeof(m_sActionName))
    {
        core.Trace("Action name: %s  - overup size of name");
        return;
    }
    pA = AttributesPointer->GetAttributeClass("ActiveActions");
    if (pA != nullptr)
        pA = pA->GetAttributeClass(actionName);
    if (pA == nullptr)
        return;
    strcpy_s(m_sActionName, actionName);
    // set texture coordinates for this action icon
    FRECT texRect;
    const int32_t curIconNum = pA->GetAttributeAsDword("IconNum", 0);
    if (curIconNum == -1)
    {
        m_bThatRealAction = false;
        return;
    }
    m_bThatRealAction = true;
    CalculateTexturePos(texRect, m_horzDiv, m_vertDiv, curIconNum);
    m_IconVertex[1].tu = m_IconVertex[0].tu = texRect.left;
    m_IconVertex[2].tu = m_IconVertex[3].tu = texRect.right;
    m_IconVertex[0].tv = m_IconVertex[2].tv = texRect.top;
    m_IconVertex[1].tv = m_IconVertex[3].tv = texRect.bottom;

    pA = AttributesPointer->GetAttributeClass("ActiveActions");
    if (pA)
    {
        m_ActionHint1.Init(rs, pA->GetAttributeClass("text1"));
        m_ActionHint2.Init(rs, pA->GetAttributeClass("text2"));
    }
    else
    {
        m_ActionHint1.Init(rs, nullptr);
        m_ActionHint2.Init(rs, nullptr);
    }
	
	const int32_t nOffset = rs->StringWidth(m_ActionHint2.sText.c_str(), m_ActionHint2.nFont, m_ActionHint2.fScale, 0);
	m_ActionBackVertex[0].pos.x = static_cast<float>(m_nActionBackCentr) - static_cast<float>(nOffset/2) - static_cast<float>(m_nActionBackWidth);

	m_ActionBackVertex[1].pos.x = static_cast<float>(m_nActionBackCentr) - static_cast<float>(nOffset/2) - static_cast<float>(m_nActionBackWidth);

	m_ActionBackVertex[2].pos.x = static_cast<float>(m_nActionBackCentr) + static_cast<float>(nOffset/2) + static_cast<float>(m_nActionBackWidth);

	m_ActionBackVertex[3].pos.x = static_cast<float>(m_nActionBackCentr) + static_cast<float>(nOffset/2) + static_cast<float>(m_nActionBackWidth);
	
}
