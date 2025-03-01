/*
 * Copyright (c) 2022, Matthew Olsson <mattco@serenityos.org>
 * Copyright (c) 2022, Julian Offenhäuser <offenhaeuser@protonmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibGfx/Font/OpenType/Font.h>
#include <LibGfx/Font/ScaledFont.h>
#include <LibGfx/Painter.h>
#include <LibPDF/CommonNames.h>
#include <LibPDF/Fonts/TrueTypeFont.h>

namespace PDF {

PDFErrorOr<void> TrueTypeFont::initialize(Document* document, NonnullRefPtr<DictObject> const& dict, float font_size)
{
    TRY(SimpleFont::initialize(document, dict, font_size));

    if (dict->contains(CommonNames::FontDescriptor)) {
        auto descriptor = MUST(dict->get_dict(document, CommonNames::FontDescriptor));
        if (descriptor->contains(CommonNames::FontFile2)) {
            auto font_file_stream = TRY(descriptor->get_stream(document, CommonNames::FontFile2));
            auto ttf_font = TRY(OpenType::Font::try_load_from_externally_owned_memory(font_file_stream->bytes()));
            float point_size = (font_size * POINTS_PER_INCH) / DEFAULT_DPI;
            m_font = adopt_ref(*new Gfx::ScaledFont(*ttf_font, point_size, point_size));
        }
    }

    return {};
}

void TrueTypeFont::draw_glyph(Gfx::Painter& painter, Gfx::FloatPoint point, float, u8 char_code, Color color)
{
    if (!m_font)
        return;

    // Account for the reversed font baseline
    auto position = point.translated(0, -m_font->baseline());
    painter.draw_glyph(position, char_code, *m_font, color);
}

}
