
#include "decoder.h"
#include "file.h"
#include "game.h"
#include "render.h"

enum {
	kInstallFileType_F3D,
	kInstallFileType_GFX,
	kInstallFileTypeCount
};

static const char *kInstallFileNames[] = {
	"INSTALL.F3D",
	"INSTALL.GFX",
};

static const int kFlagCount = 5;

static const char *kFlagNames[kFlagCount] = {
	"FR",
	"GR",
	"IT",
	"SP",
	"UK",
};

struct InstallFile {
	char name[16];
	uint32_t size;
	uint32_t offset;
};

struct InstallMeshF3D {
	int verticesCount;
	Vertex *vertices;
	uint8_t *meshData;
};

enum {
	kInstallMesh_LOGO,
	kInstallMesh_ECRAN,
	kInstallMeshCount
};

static const int kFlagWidth = 128;
static const int kFlagHeight = 96;

static const int kHeaderSize = 32; // 'mhwanh'

static const int kMaxInstallFilesCount = 48;

static const int kTexKeyFlag = 100;

struct Installer {

	Render *_render;
	InstallFile _filesTable[kMaxInstallFilesCount];
	int _filesCount;
	InstallMeshF3D _meshesTable[kInstallMeshCount];
	int _logoPitch;

	void init() {
		memset(_filesTable, 0, sizeof(_filesTable));
		_filesCount = 0;

		memset(_meshesTable, 0, sizeof(_meshesTable));

		_logoPitch = 0;

		loadInstallData(kInstallFileType_F3D);
		loadInstallData(kInstallFileType_GFX);

		uint8_t palBuf[256 * 3 + kHeaderSize + 1];
		readInstallData(kInstallFileType_GFX, "PALETTE.RAC", palBuf);
		_render->setPalette(palBuf + kHeaderSize, 0, 256);

		readInstallMeshF3D("LOGO.F3D", &_meshesTable[kInstallMesh_LOGO]);
		readInstallMeshF3D("ECRAN.F3D", &_meshesTable[kInstallMesh_ECRAN]);

		uint8_t *bitmapBuf = (uint8_t *)malloc(kFlagWidth * kFlagHeight + 256 * 3 + kHeaderSize);
		if (bitmapBuf) {
			for (int i = 0; i < kFlagCount; ++i) {
				char name[16];
				snprintf(name, sizeof(name), "FLAG_%s.RAC", kFlagNames[i]);
				readInstallData(kInstallFileType_GFX, name, bitmapBuf);
                                const int w = READ_BE_UINT16(bitmapBuf + 8);
                                const int h = READ_BE_UINT16(bitmapBuf + 10);
                                const int c = READ_BE_UINT16(bitmapBuf + 12);
				assert(w == kFlagWidth && h == kFlagHeight && c == 256);
				_render->prepareTextureLut(bitmapBuf + kHeaderSize + 256 * 3, w, h, bitmapBuf + kHeaderSize, kTexKeyFlag + i);
			}
			free(bitmapBuf);
		}
	}

	void fini() {
		for (int i = 0; i < kInstallMeshCount; ++i) {
			free(_meshesTable[i].vertices);
			free(_meshesTable[i].meshData);
		}
		memset(_meshesTable, 0, sizeof(_meshesTable));
	}

	void loadInstallData(int fileType) {
		int fileSize = 0;
		File *fp = fileOpen(kInstallFileNames[fileType], &fileSize, kFileType_INSTDATA, false);
		if (fp) {
			InstallFile *files = &_filesTable[_filesCount];
			int count = 0;
			while (1) {
				uint8_t buf[20];
				fileRead(fp, buf, sizeof(buf));
				if (fileEof(fp)) {
					break;
				}
				assert(count < kMaxInstallFilesCount);
				const int size = READ_LE_UINT32(buf + 16);
				debug(kDebug_INSTALL, "installData type %d name '%s' size %d", fileType, (const char *)buf, size);
				memcpy(files[count].name, buf, 16);
				files[count].size = size;
				files[count].offset = fileGetPos(fp);
				++count;
				fileSetPos(fp, size, kFilePosition_CUR);
			}
			fileClose(fp);
			_filesCount += count;
		}
	}

	int readInstallData(int fileType, const char *name, uint8_t *dst) {
		for (int i = 0; i < _filesCount; ++i) {
			const InstallFile *f = &_filesTable[i];
			if (strcasecmp(name, f->name) == 0) {
				int fileSize = 0;
				File *fp = fileOpen(kInstallFileNames[fileType], &fileSize, kFileType_INSTDATA);
				fileSetPos(fp, f->offset, kFilePosition_SET);
				int decompressedSize = 0;
				uint8_t *buf = (uint8_t *)malloc(f->size);
				if (buf) {
					fileRead(fp, buf, f->size);
					decompressedSize = READ_LE_UINT32(buf);
					decodeRAC(buf + 4, dst, decompressedSize);
					free(buf);
				}
				fileClose(fp);
				return decompressedSize;
			}
		}
		return 0;
	}

	void readInstallMeshF3D(const char *name, InstallMeshF3D *m3d) {
		uint8_t buf[16384];
		const int size = readInstallData(kInstallFileType_F3D, name, buf);
		if (size != 0) {
			int offset = 0;
			assert(READ_LE_UINT16(buf + offset) == 1); offset += 2;
			m3d->verticesCount = READ_LE_UINT16(buf + offset); offset += 2;
			m3d->vertices = (Vertex *)malloc(m3d->verticesCount * sizeof(Vertex));
			if (m3d->vertices) {
				for (int i = 0; i < m3d->verticesCount; ++i) {
					const int x = (int16_t)READ_LE_UINT16(buf + offset); offset += 2;
					const int y = (int16_t)READ_LE_UINT16(buf + offset); offset += 2;
					const int z = (int16_t)READ_LE_UINT16(buf + offset); offset += 2;
					offset += 3 * sizeof(uint16_t); /* nx, ny, nz */
					m3d->vertices[i].x = x;
					m3d->vertices[i].y = y / 2;
					m3d->vertices[i].z = z;
				}
				if (0 && m3d->verticesCount > 0) { /* compute bounding box */
					int xmin, xmax;
					xmin = xmax = m3d->vertices[0].x;
					int ymin, ymax;
					ymin = ymax = m3d->vertices[0].y;
					int zmin, zmax;
					zmin = zmax = m3d->vertices[0].z;
					for (int i = 1; i < m3d->verticesCount; ++i) {
						const int x = m3d->vertices[i].x;
						if (xmin > x) xmin = x;
						if (xmax < x) xmax = x;
						const int y = m3d->vertices[i].y;
						if (ymin > y) ymin = y;
						if (ymax < y) ymax = y;
						const int z = m3d->vertices[i].z;
						if (zmin > z) zmin = z;
						if (zmax < z) zmax = z;
					}
					debug(kDebug_INSTALL, "bounds [%d,%d;%d,%d;%d;%d]", xmin, xmax, ymin, ymax, zmin, zmax);
				}
			}
			const int dataSize = size - m3d->verticesCount * 6 * sizeof(uint16_t) - 4;
			m3d->meshData = (uint8_t *)malloc(dataSize);
			if (m3d->meshData) {
				memcpy(m3d->meshData, buf + offset, dataSize);
			}
			debug(kDebug_INSTALL, "mesh '%s' vertices %d dataSize %d", name, m3d->verticesCount, dataSize);
		}
	}

	void drawInstallMeshF3D(InstallMeshF3D *m3d, int texKey = 0) {
		int offset = 0;
		while (1) {
			const int count = READ_LE_UINT16(m3d->meshData + offset); offset += 2;
			if (count == 0) {
				break;
			}
			int color = READ_LE_UINT16(m3d->meshData + offset); offset += 2;
			Vertex polygons[256];
			assert(count < 256);
			for (int i = 0; i < count; ++i) {
				const int index = READ_LE_UINT16(m3d->meshData + offset); offset += 2;
				assert(index < m3d->verticesCount);
				polygons[i] = m3d->vertices[index];
			}
			assert((color >> 8) == 0x80);
			color &= 255;
			if (color == 255) {
				_render->drawPolygonTexture(polygons, count, 0, 0, kFlagWidth, kFlagHeight, texKey);
			} else {
				_render->drawPolygonFlat(polygons, count, color);
			}
		}
	}

	void update(const PlayerInput &inp) {
		_render->clearScreen();
		_render->setupProjection(kProjInstall);
		_render->setIgnoreDepth(false);

		_render->beginObjectDraw(0, 0, 0, _logoPitch, kPosShift);
		drawInstallMeshF3D(&_meshesTable[kInstallMesh_LOGO]);
		_render->endObjectDraw();

		for (int i = 0; i < kFlagCount; ++i) {
			_render->beginObjectDraw(0, 0, 0, i * 1024 / kFlagCount, kPosShift);
			drawInstallMeshF3D(&_meshesTable[kInstallMesh_ECRAN], kTexKeyFlag + i);
			_render->endObjectDraw();
		}

		_logoPitch += 16;
		_logoPitch &= 1023;
	}
};

static Installer _installer;

void Game::initInstaller() {
	_installer._render = _render;
	_installer.init();
}

void Game::finiInstaller() {
	_installer.fini();
}

void Game::doInstaller() {
	_installer.update(inp);
}
