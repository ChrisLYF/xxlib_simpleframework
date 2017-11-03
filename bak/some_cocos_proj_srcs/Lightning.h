#ifndef _LIGHTNING_H_
#define _LIGHTNING_H_

#include "cocos2d.h"


//��������ʾ��
class Lightning : public cocos2d::Layer
{
public:
    typedef std::vector<Vec3>  PointVec;

public:
	//����
	Lightning();
	//����
	~Lightning(void);

	//��ʼ��
    virtual bool init();  
	//����
    CREATE_FUNC(Lightning);

    //���ػ��ƴ�����
	virtual void draw(Renderer* renderer, const Mat4 &transform, uint32_t flags);

    bool setPoints( const std::vector<Vec3> &pts);

	//���ÿ��
	void setWidth( float f )
	{
		m_Width = f;
	}

	float getWidth(void) const
	{
		return m_Width;
	}

	//���õ�����Ƭ���񳤣�ԽС����Խƽ������ֵ����С���ܴ���Ч������
	void setStep( float f )
	{
		m_Step = f;
	}

	float getStep(void) const
	{
		return m_Step;
	}

    void setSpeed( float f )
	{
		m_spd = f;
	}

	float getSpeed(void) const
	{
		return m_spd;
	}

    // ���������ȣ����������ֵ���Ա���������������ѹ
	void setTextureLength( float f )
	{
		m_TextureLength = f;
	}
	float getTextureLength(void) const
	{
		return m_TextureLength;
	}

    Vec3 getCenter(void);

private:
    void updateCenter();

	//��ʼ��
	void init( const std::string& materialPath );
	//����
	void updateMesh(void);

private:
	float						  m_Time;			// ��
	float						  m_Width;			// ��
	float						  m_Step;			// ������Ƭ���񳤣�ԽС����Խƽ������ֵ����С���ܴ���Ч������
    float						  m_TextureLength;	// ���������ȣ����������ֵ���Ա���������������ѹ
    float						  m_spd;
    int                           m_fps;

    Vec3                          m_center;            // ��Χ�� ���ĵ�
    bool                          m_need_rebuild_mesh; // �Ƿ� ��Ҫ �ؽ� mesh ���ݣ� �� step �� points �仯ʱ������true
    bool                          m_need_reset_cmd;

	Texture2D                    *m_Texture;			//����
	Mesh						 *m_RenderMesh;		//��Ⱦģ��
	BlendFunc					  m_BlendFunc;		//��Ⱦ״̬

    PointVec                      m_points;            // ������
    std::vector<float>            m_vertexs;    //
    std::vector<unsigned short>   m_indinces;   //
	MeshCommand 				  m_pMeshCommand;	//��Ⱦ����

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_WP8)
    EventListenerCustom *_backgroundListener;
#endif
};

#endif	//#define _LIGHTNING_H_