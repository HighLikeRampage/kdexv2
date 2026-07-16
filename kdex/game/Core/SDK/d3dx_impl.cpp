#include <D3dx9math.h>
#include <cmath>

D3DXVECTOR3* WINAPI D3DXVec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV)
{
    float len = sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
    if (len > 0.0f) {
        float inv = 1.0f / len;
        pOut->x = pV->x * inv;
        pOut->y = pV->y * inv;
        pOut->z = pV->z * inv;
    } else {
        pOut->x = pV->x;
        pOut->y = pV->y;
        pOut->z = pV->z;
    }
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixTranspose(D3DXMATRIX* pOut, const D3DXMATRIX* pM)
{
    D3DXMATRIX tmp = *pM;
    pOut->_11 = tmp._11; pOut->_12 = tmp._21; pOut->_13 = tmp._31; pOut->_14 = tmp._41;
    pOut->_21 = tmp._12; pOut->_22 = tmp._22; pOut->_23 = tmp._32; pOut->_24 = tmp._42;
    pOut->_31 = tmp._13; pOut->_32 = tmp._23; pOut->_33 = tmp._33; pOut->_34 = tmp._43;
    pOut->_41 = tmp._14; pOut->_42 = tmp._24; pOut->_43 = tmp._34; pOut->_44 = tmp._44;
    return pOut;
}

D3DXVECTOR4* WINAPI D3DXVec3Transform(D3DXVECTOR4* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM)
{
    pOut->x = pV->x * pM->_11 + pV->y * pM->_21 + pV->z * pM->_31 + pM->_41;
    pOut->y = pV->x * pM->_12 + pV->y * pM->_22 + pV->z * pM->_32 + pM->_42;
    pOut->z = pV->x * pM->_13 + pV->y * pM->_23 + pV->z * pM->_33 + pM->_43;
    pOut->w = pV->x * pM->_14 + pV->y * pM->_24 + pV->z * pM->_34 + pM->_44;
    return pOut;
}
