#include "lua.hpp"
#include <Windows.h>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include "Trans.h"
#include"PixelSort_struct.h"
#include "UtilFunc.h"

using namespace std;

float(*const comp_func[])(Pixel_BGRA*) = {
	UtilFunc::comp_luminance,
	UtilFunc::comp_average,
	UtilFunc::comp_multiply,
	UtilFunc::comp_min,
	UtilFunc::comp_max,
	UtilFunc::comp_xor
};

inline unsigned int calpos(unsigned int x, unsigned int y, unsigned int w, byte mag){
	return y*mag*w + min(UtilFunc::clamp(x, 0, w)*mag, w);
};

int PixelSort_Func(lua_State *L){
	int b = UtilFunc::clamp((int)lua_tointeger(L, 1), 0, 4096);
	int d = UtilFunc::clamp((int)lua_tointeger(L, 2), 0, 4096);
	int r = UtilFunc::clamp((int)lua_tointeger(L, 3), 1, 4);
	byte dsize = (int)lua_tointeger(L, 4);
	bool conf = (bool)lua_toboolean(L, 5);
	int stretch_direction = UtilFunc::clamp((int)lua_tointeger(L, 6), 1, 4);
	float stretch_length = UtilFunc::clamp((float)lua_tonumber(L, 7)/100.f, 0.f, 1.f);
	byte comp = UtilFunc::clamp((byte)lua_tonumber(L, 8)-1, 0, 5);
	bool bi = (bool)lua_tonumber(L, 9);
	byte sr = r;

	lua_getglobal(L, "obj");
	lua_getfield(L, -1, "getpixeldata");
	lua_call(L, 0, 3);
	int h = lua_tointeger(L, -1);
	lua_pop(L, 1);
	int w = lua_tointeger(L, -1);
	lua_pop(L, 1);
	Pixel_BGRA *sourcedata = new Pixel_BGRA[w*h];
	memcpy(sourcedata, lua_touserdata(L, -1), sizeof(Pixel_BGRA)*w*h);
	lua_pop(L, 1);
	lua_getfield(L, -1, "effect");
	lua_pushstring(L, "���~�i���X�L�[");
	lua_pushstring(L, "type");
	lua_pushnumber(L, 4);
	lua_pushstring(L, "��P�x");
	lua_pushnumber(L, b);
	lua_pushstring(L, "�ڂ���");
	lua_pushnumber(L, d);
	lua_call(L, 7, 0);
	lua_getfield(L, -1, "getpixeldata");
	lua_call(L, 0, 3);
	Pixel_BGRA *data = (Pixel_BGRA*)lua_touserdata(L, -3);
	lua_pop(L, 1);
	lua_pop(L, 1);
	lua_pop(L, 1);
	const int size = w*h;
	int dw = ceil((float)w / (float)dsize);
	int dh = ceil((float)h / (float)dsize);

	Pixel_BGRA *sdata = new Pixel_BGRA[dw*dh];
	Pixel_BGRA *odata = (bi) ? sourcedata : new Pixel_BGRA[size];

	isize orig_isize(w, h);
	isize shr_isize(dw, dh);

	if (r<3){
		Trans::Shrink_r(data, sdata, orig_isize, shr_isize, dsize);
	}else{
		Trans::Shrink(data, sdata, orig_isize, shr_isize, dsize);
	}
	Pixel_map *map = new Pixel_map[dw*dh];
	for (unsigned long i = 0; i < dw*dh; i++) {
		Pixel_map *m = &map[i];
		m->pix = sdata[i];
		m->sort_key = comp_func[comp](&m->pix);
	}


	int vw =  (r < 3) ? dh : dw;
	int vh =  (r < 3) ? dw : dh;
	if(!conf){
		for (unsigned long y = 0; y < vh; y++){
			vector<Pixel_map> pix;
			unsigned long x = 0;
			unsigned long offsetpos = 0;
			while (x < vw) {
				unsigned long pos = y*vw + x;
				byte a = map[pos].pix.a;
				if (a){
					if (!pix.size()){
						//OutputDebugString("emp");
						offsetpos = pos;
					}
					pix.push_back(map[pos]);
				}else if(pix.size()){
					//OutputDebugString(to_string(pix.size()).c_str());
					if (sr == 2 || sr == 4) {
						sort(pix.begin(), pix.end(), greater<Pixel_map>());// , [](const Pixel_map& a, const Pixel_map& b) {return (a.sort_key < b.sort_key); });
					}
					else {
						sort(pix.begin(), pix.end());// , [](const Pixel_map& a, const Pixel_map& b) {return (a.sort_key > b.sort_key); });
					}
					for (unsigned long i = 0; i < pix.size(); i++) {
						sdata[offsetpos + i] = pix[i].pix;
					}
					pix.clear();
					offsetpos = 0;
				}
				if (x==(vw-1)){
					//OutputDebugString(to_string(pix.size()).c_str());
					if (sr == 2 || sr == 4) {
						sort(pix.begin(), pix.end(), greater<Pixel_map>());// , [](const Pixel_map& a, const Pixel_map& b) {return (a.sort_key < b.sort_key); });
					}
					else {
						sort(pix.begin(), pix.end());// , [](const Pixel_map& a, const Pixel_map& b) {return (a.sort_key > b.sort_key); });
					}
					for (unsigned long i = 0; i < pix.size(); i++) {
						sdata[offsetpos + i] = pix[i].pix;
					}
					pix.clear();
					offsetpos = 0;
				}
				x++;
			}
		}
	} else {
		for (unsigned long i = 0; i < vw*vh; i++){
			Pixel_BGRA p = sdata[i];
			if (p.a){
				p.r = 0;
				p.g = 255;
				p.b = 0;
				sdata[i] = p;
			}
		}
	}
	if (bi) odata = sourcedata;

	if (r < 3){
		Trans::Restore_r(sdata, odata, shr_isize, orig_isize, dsize, stretch_direction, stretch_length, bi);
	}
	else
	{
		Trans::Restore(sdata, odata, shr_isize, orig_isize, dsize, stretch_direction, stretch_length, bi);
	}

	lua_getglobal(L, "obj");
	lua_getfield(L, -1, "putpixeldata");
	lua_pushlightuserdata(L, odata);
	lua_call(L, 1, 0);


	delete[] sdata;
	delete[] map;
	if (!bi)delete[] odata;
	delete[] sourcedata;
	return 0;
}

int Instructions(lua_State *L) {
	const byte Max_page = 2;
	byte pagenum = UtilFunc::clamp((byte)lua_tointeger(L, 1)-1, 0, Max_page-1);

	string version = "1.51";
	string Inst[Max_page];
	Inst[0] =
			"��P�x : �摜�؂�o���̊�ƂȂ�P�x�ł��B\n"
			"�P�x�� : ��P�x����̋P�x�̕��ł��B\n"
			"���� : ���ƂȂ����o�[�W�������猸����4��ނɂȂ�܂����B�c��2�������Ȃ̂ł��ꂪ��Ԃ��Ǝv���܂��B\n"
			"�����L�΂� : �\�[�g��̉摜���w�肵�������Ɉ������΂��܂��B\n"
			"���L���� : �����L�΂��̕������E�A���A���A���4��ނ���I�ׂ܂��B\n"
			"�\�[�g� : �\�[�g�̊�l�̌v�Z����I�ׂ܂�(�S6���)�B\n"
			"�T�C�Y : �s�N�Z���̃��U�C�N���̃T�C�Y�ł��B��������Ώ������قǏd���ł��B�t�ɑ傫���ƌ��\�y���ł��B\n"
			"�̈���m�F : �I�������͈͂�΂ŕ\�����܂��B\n"
			"���摜�ƍ��� : �\�[�X�̉摜�����ɍ������܂��B\n";
	Inst[1] =
			"<s36>�\�[�g��� [1-6]\n<s>"
			"  1 : Luminance (�P�x�)\n"
			"  2 : Average (RGB�̕���)\n"
			"  3 : Multiply (���K������RGB����Z)\n"
			"  4 : Min (RGB�̍ŏ��l)\n"
			"  5 : Max (RGB�̍ő�l)\n"
			"  6 : XOR (RGB��XOR���Z)";

	string info = "<s40>PixelSorter_s �ȈՐ����� - Page : " + to_string(pagenum+1) + "<s>\nVersion : " + version + "\n\n";
	info += Inst[pagenum];
	lua_getglobal(L, "obj");

	lua_getfield(L, -1, "effect");
	lua_call(L, 0, 0);

	lua_getfield(L, -1, "draw");
	lua_call(L, 0, 0);

	lua_getfield(L, -1, "load");
	lua_pushstring(L, "figure");
	lua_pushstring(L, "�l�p�`");
	lua_pushinteger(L, 0);
	lua_pushinteger(L, 1920);
	lua_call(L, 4, 0);

	lua_getfield(L, -1, "effect");
	lua_pushstring(L, "���T�C�Y");
	lua_pushstring(L, "Y");
	lua_pushnumber(L, 55);
	lua_call(L, 3, 0);

	lua_getfield(L, -1, "draw");
	lua_pushinteger(L, 0);
	lua_pushinteger(L, 0);
	lua_pushinteger(L, 0);
	lua_pushinteger(L, 1);
	lua_pushnumber(L, 0.75);
	lua_call(L, 5, 0);

	lua_getfield(L, -1, "load");
	lua_pushstring(L, "figure");
	lua_pushstring(L, "�l�p�`");
	lua_pushinteger(L, 0xaaaaaa);
	lua_pushinteger(L, 1870);
	lua_call(L, 4, 0);

	lua_getfield(L, -1, "effect");
	lua_pushstring(L, "���T�C�Y");
	lua_pushstring(L, "Y");
	lua_pushnumber(L, 50);
	lua_call(L, 3, 0);

	lua_getfield(L, -1, "draw");
	lua_pushinteger(L, 0);
	lua_pushinteger(L, 0);
	lua_pushinteger(L, 0);
	lua_pushinteger(L, 1);
	lua_pushnumber(L, 0.3);
	lua_call(L, 5, 0);

	lua_getfield(L, -1, "setfont");
	lua_pushstring(L, "���C���I");
	lua_pushinteger(L, 30);
	lua_pushinteger(L, 1);
	lua_pushinteger(L, 0xffffff);
	lua_pushinteger(L, 0x444444);
	lua_call(L, 5, 0);

	lua_getfield(L, -1, "load");
	lua_pushstring(L, "text");
	lua_pushstring(L, info.c_str());
	lua_call(L, 2, 0);

	lua_getfield(L, -1, "draw");
	lua_pushinteger(L, 0);
	lua_pushinteger(L, 0);
	lua_pushinteger(L, 0);
	lua_pushinteger(L, 1);
	lua_pushnumber(L, 1);
	lua_call(L, 5, 0);

	return 0;
}

static luaL_Reg PixelSorter_s[] = {
	{ "PixelSort", PixelSort_Func },
	{ "Instructions", Instructions },
	{ NULL, NULL }
};

/*
������dll���`���܂�
�ʂ̂��̂����ꍇ��
luaopen_PixelSort
�̕�����V�������O�ɕς��Ă�������
*/
extern "C"{
	__declspec(dllexport) int luaopen_PixelSorter_s(lua_State *L) {
		luaL_register(L, "PixelSorter_s", PixelSorter_s);
		return 1;
	}
}
