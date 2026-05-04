#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "engine/clx_sprite.hpp"

namespace devilution {

namespace placeholder_clx_detail {

inline void WriteLe16(uint8_t *dst, uint16_t value)
{
	dst[0] = static_cast<uint8_t>(value & 0xFF);
	dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

inline void WriteLe32(uint8_t *dst, uint32_t value)
{
	dst[0] = static_cast<uint8_t>(value & 0xFF);
	dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
	dst[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
	dst[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

inline void PutPixel(std::vector<uint8_t> &pixels, uint16_t width, uint16_t height, int x, int y, uint8_t color)
{
	if (x < 0 || y < 0 || x >= width || y >= height)
		return;
	pixels[static_cast<size_t>(y) * width + x] = color;
}

inline bool GlyphPixel(char ch, int x, int y)
{
	// 3x5 compact debug font, enough to stamp PLACEHOLDER inside the fallback sprite.
	static constexpr uint8_t P[5] = { 0b110, 0b101, 0b110, 0b100, 0b100 };
	static constexpr uint8_t L[5] = { 0b100, 0b100, 0b100, 0b100, 0b111 };
	static constexpr uint8_t A[5] = { 0b010, 0b101, 0b111, 0b101, 0b101 };
	static constexpr uint8_t C[5] = { 0b011, 0b100, 0b100, 0b100, 0b011 };
	static constexpr uint8_t E[5] = { 0b111, 0b100, 0b110, 0b100, 0b111 };
	static constexpr uint8_t H[5] = { 0b101, 0b101, 0b111, 0b101, 0b101 };
	static constexpr uint8_t O[5] = { 0b010, 0b101, 0b101, 0b101, 0b010 };
	static constexpr uint8_t D[5] = { 0b110, 0b101, 0b101, 0b101, 0b110 };
	static constexpr uint8_t R[5] = { 0b110, 0b101, 0b110, 0b101, 0b101 };

	const uint8_t *glyph = nullptr;
	switch (ch) {
	case 'P': glyph = P; break;
	case 'L': glyph = L; break;
	case 'A': glyph = A; break;
	case 'C': glyph = C; break;
	case 'E': glyph = E; break;
	case 'H': glyph = H; break;
	case 'O': glyph = O; break;
	case 'D': glyph = D; break;
	case 'R': glyph = R; break;
	default: return false;
	}
	return (glyph[y] & (1u << (2 - x))) != 0;
}

inline void DrawText(std::vector<uint8_t> &pixels, uint16_t width, uint16_t height, int x, int y, const char *text, uint8_t color)
{
	for (const char *ch = text; *ch != '\0'; ++ch) {
		if (*ch == ' ') {
			x += 4;
			continue;
		}
		for (int gy = 0; gy < 5; ++gy) {
			for (int gx = 0; gx < 3; ++gx) {
				if (GlyphPixel(*ch, gx, gy))
					PutPixel(pixels, width, height, x + gx, y + gy, color);
			}
		}
		x += 4;
	}
}

inline std::vector<uint8_t> BuildPlaceholderPixels(uint16_t width, uint16_t height)
{
	std::vector<uint8_t> pixels(static_cast<size_t>(width) * height, 0);

	// Debug frame.
	for (int x = 0; x < width; ++x) {
		PutPixel(pixels, width, height, x, 0, 4);
		PutPixel(pixels, width, height, x, height - 1, 4);
	}
	for (int y = 0; y < height; ++y) {
		PutPixel(pixels, width, height, 0, y, 4);
		PutPixel(pixels, width, height, width - 1, y, 4);
	}

	const int cx = width / 2;
	const int headCy = std::max(12, height / 4);
	const int bodyTop = std::max(6, headCy - 10);
	const int bodyBottom = std::max(bodyTop + 18, static_cast<int>(height) - 22);

	// Ghost body silhouette.
	for (int y = bodyTop; y <= bodyBottom; ++y) {
		for (int x = 2; x < width - 2; ++x) {
			const int dx = x - cx;
			const int dy = y - headCy;
			const int radius = std::max(10, static_cast<int>(width) / 5);
			const bool head = dx * dx + dy * dy <= radius * radius;
			const int halfBody = std::max(10, static_cast<int>(width) / 4);
			const bool body = y >= headCy && y <= bodyBottom && std::abs(dx) <= halfBody;
			const bool scallop = y > bodyBottom - 5 && ((x / 5) % 2 == 0);
			if (head || (body && !scallop))
				PutPixel(pixels, width, height, x, y, 15);
		}
	}

	// Eyes.
	PutPixel(pixels, width, height, cx - 5, headCy - 2, 0);
	PutPixel(pixels, width, height, cx + 5, headCy - 2, 0);
	PutPixel(pixels, width, height, cx - 5, headCy - 1, 0);
	PutPixel(pixels, width, height, cx + 5, headCy - 1, 0);

	// Belly shadow.
	for (int y = headCy + 7; y < bodyBottom - 3; ++y) {
		for (int x = cx - 8; x <= cx + 8; ++x) {
			if (((x + y) % 5) == 0)
				PutPixel(pixels, width, height, x, y, 8);
		}
	}

	DrawText(pixels, width, height, std::max(2, cx - 10), std::max(2, static_cast<int>(height) - 17), "PLACE", 14);
	DrawText(pixels, width, height, std::max(2, cx - 12), std::max(2, static_cast<int>(height) - 10), "HOLDER", 14);

	return pixels;
}

inline std::vector<uint8_t> EncodePlaceholderClxList(uint16_t width, uint16_t height)
{
	width = std::clamp<uint16_t>(width, 8, 192);
	height = std::clamp<uint16_t>(height, 8, 192);

	const std::vector<uint8_t> pixels = BuildPlaceholderPixels(width, height);
	std::vector<uint8_t> pixelData;
	pixelData.reserve(static_cast<size_t>(width + 1) * height);

	for (uint16_t y = 0; y < height; ++y) {
		uint16_t x = 0;
		while (x < width) {
			const uint8_t run = static_cast<uint8_t>(std::min<uint16_t>(64, width - x));
			pixelData.push_back(static_cast<uint8_t>(256 - run)); // CLX opaque-pixels command.
			pixelData.insert(pixelData.end(), pixels.begin() + static_cast<size_t>(y) * width + x,
			    pixels.begin() + static_cast<size_t>(y) * width + x + run);
			x += run;
		}
	}

	constexpr uint32_t ListHeaderSize = 12; // frame count + first offset + end offset.
	constexpr uint32_t FrameHeaderSize = 6;
	const uint32_t totalSize = ListHeaderSize + FrameHeaderSize + static_cast<uint32_t>(pixelData.size());

	std::vector<uint8_t> data(totalSize);
	WriteLe32(&data[0], 1); // one sprite
	WriteLe32(&data[4], ListHeaderSize);
	WriteLe32(&data[8], totalSize);
	WriteLe16(&data[ListHeaderSize + 0], FrameHeaderSize);
	WriteLe16(&data[ListHeaderSize + 2], width);
	WriteLe16(&data[ListHeaderSize + 4], height);
	std::copy(pixelData.begin(), pixelData.end(), data.begin() + ListHeaderSize + FrameHeaderSize);
	return data;
}

inline std::unique_ptr<uint8_t[]> CopyToOwnedBuffer(const std::vector<uint8_t> &buffer)
{
	std::unique_ptr<uint8_t[]> owned { new uint8_t[buffer.size()] };
	std::memcpy(owned.get(), buffer.data(), buffer.size());
	return owned;
}

} // namespace placeholder_clx_detail

inline OwnedClxSpriteList MakePlaceholderClxSpriteList(uint16_t width = 64, uint16_t height = 64)
{
	std::vector<uint8_t> data = placeholder_clx_detail::EncodePlaceholderClxList(width, height);
	return OwnedClxSpriteList { placeholder_clx_detail::CopyToOwnedBuffer(data) };
}

inline OwnedClxSpriteSheet MakePlaceholderClxSpriteSheet(uint16_t numLists = 8, uint16_t width = 64, uint16_t height = 64)
{
	numLists = std::max<uint16_t>(1, numLists);
	const std::vector<uint8_t> list = placeholder_clx_detail::EncodePlaceholderClxList(width, height);

	std::vector<uint8_t> sheet(static_cast<size_t>(numLists) * 4);
	for (uint16_t i = 0; i < numLists; ++i) {
		placeholder_clx_detail::WriteLe32(&sheet[static_cast<size_t>(i) * 4], static_cast<uint32_t>(sheet.size()));
		sheet.insert(sheet.end(), list.begin(), list.end());
	}

	return OwnedClxSpriteSheet { placeholder_clx_detail::CopyToOwnedBuffer(sheet), numLists };
}

inline OwnedClxSpriteListOrSheet MakePlaceholderClxListOrSheet(uint16_t width = 64, uint16_t height = 64)
{
	return OwnedClxSpriteListOrSheet { MakePlaceholderClxSpriteList(width, height) };
}

inline OwnedClxSpriteListOrSheet MakePlaceholderClxSheetOrList(uint16_t numLists = 8, uint16_t width = 64, uint16_t height = 64)
{
	return OwnedClxSpriteListOrSheet { MakePlaceholderClxSpriteSheet(numLists, width, height) };
}

} // namespace devilution
