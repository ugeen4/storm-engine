#pragma once

#include "bi_utils.h"

class ILogAndActions : public Entity
{
    VDX9RENDER *rs;

    struct STRING_DESCR
    {
        char *str;
        float offset;
        float alpha;
        STRING_DESCR *next;
    };

  public:
    ILogAndActions(ILogAndActions &&) = delete;
    ILogAndActions(const ILogAndActions &) = delete;
    ILogAndActions();
    ~ILogAndActions() override;
    bool Init() override;
    void Execute(uint32_t delta_time);
    void Realize(uint32_t delta_time);
    uint64_t ProcessMessage(MESSAGE &message) override;

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        switch (stage)
        {
        case Stage::execute:
            Execute(delta);
            break;
        case Stage::realize:
            Realize(delta);
            break;
            /*case Stage::lost_render:
              LostRender(delta); break;
            case Stage::restore_render:
              RestoreRender(delta); break;*/
        }
    }

  protected:
    void Create(bool bFastComShow, bool bLogStringShow);
    void ActionChange(bool bFastComShow, bool bLogStringShow);
    void Release();

    void SetString(const char *str, bool immortal);
    void SetPicInfo(const char *str);
	void SetLevelUp();
    void SetAction(const char *actionName);

  protected:
    // log stings parameters
    // -----------------------
    int32_t m_nWindowWidth;  // X window size
    int32_t m_nWindowHeight; // Y window size
    int32_t m_nWindowLeft;
    int32_t m_nWindowRight;
    int32_t m_nWindowUp;
    int32_t m_fontID;
    float m_fFontScale;
    uint32_t m_dwColor;
    int32_t m_nStringBegin;
    int32_t m_nStringOffset;
    float m_fShiftSpeed;
    float m_fBlendSpeed;
    STRING_DESCR *m_sRoot;

    // Action parameters
    //-------------------
    bool m_bThatRealAction;
    int32_t m_nIconWidth;  // X icon size
    int32_t m_nIconHeight; // Y icon size
    int32_t m_nIconLeft;
    int32_t m_nIconUp;
    int32_t m_idIconTexture;
    int32_t m_horzDiv;
    int32_t m_vertDiv;
    BI_ONETEXTURE_VERTEX m_IconVertex[4];
    char m_sActionName[64];
    char m_sOldActionName[64];

    bool m_bShowActiveCommand;
    bool m_bShowLogStrings;
    bool m_bDontShowAll;

    int32_t m_nTimeCounter;

    BITextInfo m_ActionHint1;
    BITextInfo m_ActionHint2;
	
	// backimage for actions
	int32_t m_nActionBackWidth;  // X texture size
    int32_t m_nActionBackHeight; // Y texture size
    int32_t m_nActionBackCentr;
    int32_t m_nActionBackUp;
    int32_t m_idActionBackTexture;
	uint32_t m_dwActionBackColor;
	std::string ALtype;
	BI_COLOR_VERTEX m_ActionBackVertex[4];
	
	
	
	// pic info 
    // strings parameters-----------------------
    int32_t m_nPicInfoWindowWidth;  // X window size
    int32_t m_nPicInfoWindowHeight; // Y window size
    int32_t m_nPicInfoWindowLeft;
    int32_t m_nPicInfoWindowRight;
    int32_t m_nPicInfoWindowUp;
    int32_t m_PicInfofontID;
    float m_fPicInfoFontScale;
    uint32_t m_dwPicInfoColor;
    int32_t m_nPicInfoStringBegin;
    int32_t m_nPicInfoStringOffset;
    float m_fPicInfoShiftSpeed;
    float m_fPicInfoBlendSpeed;
    STRING_DESCR *m_pRoot;
	
	// backimage param
	uint32_t m_dwPicInfoMaxColor;
    uint32_t m_dwPicInfoMinColor;
	int32_t m_nPicInfoBackWidth;  // X texture size
    int32_t m_nPicInfoBackHeight; // Y texture size
    int32_t m_nPicInfoBackLeft;
    int32_t m_nPicInfoBackUp;
    int32_t m_idPicInfoBackTexture;
	BI_COLOR_VERTEX m_PicInfoBackVertex[4];
	BI_COLOR_VERTEX m_PicInfoBackVertex1[4];
	BI_COLOR_VERTEX m_PicInfoBackVertex2[4];
	float AlphaBI;
	float AlphaBI2;
	float AlphaBI3;
	
	// icon param
	int32_t m_nPicInfoIconWidth;  // X texture size
    int32_t m_nPicInfoIconHeight; // Y texture size
    int32_t m_nPicInfoIconLeft;
    int32_t m_nPicInfoIconUp;
    int32_t m_idPicInfoIconTexture;
    int32_t m_nPicInfoIconhorzDiv;
    int32_t m_nPicInfoIconvertDiv;
	int8_t m_nPicInfoIconIndex;
    BI_COLOR_VERTEX m_PicInfoIconVertex[4];
	BI_COLOR_VERTEX m_PicInfoIconVertex1[4];
	BI_COLOR_VERTEX m_PicInfoIconVertex2[4];
	
	int8_t PIstage;
	
	// time speed
	// texture
	bool bShowTimeSpeed;
    int32_t m_nTimeSpeedIconWidth;  // X icon size
    int32_t m_nTimeSpeedIconHeight; // Y icon size
    int32_t m_nTimeSpeedIconLeft;
    //int32_t m_nTimeSpeedIconRight;
    int32_t m_nTimeSpeedIconUp;
    int32_t m_idTimeSpeedIconTexture;
	uint32_t m_dwTimeSpeedIconColor;
    BI_COLOR_VERTEX m_TimeSpeedIconVertex[4];
	// text
    int32_t m_nTimeSpeedLeft;
    //int32_t m_nTimeSpeedRight;
    int32_t m_nTimeSpeedUp;
    int32_t m_TimeSpeedfontID;
    float m_fTimeSpeedFontScale;
    uint32_t m_dwTimeSpeedColor;
    std::string TimeSpeedText;
	
	
	// level up
	//backimage
	uint32_t m_dwLUBackColor;
    //uint32_t m_dwPicInfoMinColor;
	int32_t m_nLUBackWidth;  // X texture size
    int32_t m_nLUBackHeight; // Y texture size
    int32_t m_nLUBackLeft;
    int32_t m_nLUBackUp;
    int32_t m_idLUBackTexture;
	BI_COLOR_VERTEX m_LUBackVertex[4];
	float AlphaLU;
	
	// string
	int32_t m_nLUStringLeft;
    int32_t m_nLUStringUp;
    int32_t m_LUStringfontID;
    float m_fLUStringFontScale;
    uint32_t m_dwLUStringColor;
	float m_fLUStringSpeed;
    std::string LUStringText;
	
	//rank
	int32_t m_nLURankLeft;
    int32_t m_nLURankUp;
    int32_t m_LURankfontID;
    float m_fLURankFontScale;
    uint32_t m_dwLURankColor;
	//float m_fLURankSpeed;
    std::string LURankText;
	
	
	
};
