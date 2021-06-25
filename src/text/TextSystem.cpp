#pragma once

#ifdef USING_SDFTEXT

#include "cinder/app/App.h"
#include "text/TextSystem.h"

using namespace sitara::ecs;

TextSystem::TextSystem() {
}

TextSystem::~TextSystem() {
}

void TextSystem::configure(entityx::EntityManager& entities, entityx::EventManager& events) {
}

void TextSystem::update(entityx::EntityManager& entities, entityx::EventManager& events, entityx::TimeDelta dt) {
}

void TextSystem::registerFont(const std::string& name, const std::filesystem::path& path, float fontSize) {
	std::string sdft_filename = path.stem().string();
	sdft_filename += ".sdft";
	auto fontInstance = ci::gl::SdfText::create(ci::app::getAssetPath("") / "sdft" / sdft_filename, ci::gl::SdfText::Font(path.string(), fontSize));
	mFontInstances.insert(std::pair<std::string, ci::gl::SdfTextRef>(name, fontInstance));
}

std::vector<std::pair<ci::gl::SdfText::Font::Glyph, ci::vec2>> TextSystem::getGlyphPlacements(const std::string& fontName, const std::string& str, const ci::gl::SdfText::DrawOptions& options) {
	ci::gl::SdfTextRef fontRenderer = mFontInstances[fontName];
	return fontRenderer->getGlyphPlacements(str, options);
}

void TextSystem::drawGlyphs(const std::string& fontName, std::vector<std::pair<ci::gl::SdfText::Font::Glyph, ci::vec2>> glyphPlacements, const ci::vec2& baseline, const ci::gl::SdfText::DrawOptions& options) {
	ci::gl::SdfTextRef fontRenderer = mFontInstances[fontName];
	fontRenderer->drawGlyphs(glyphPlacements, baseline, options);
}

#endif