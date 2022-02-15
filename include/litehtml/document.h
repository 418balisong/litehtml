#ifndef LH_DOCUMENT_H
#define LH_DOCUMENT_H

#include "style.h"
#include "types.h"
#include "context.h"
#include "el_text.h"
#include "el_space.h"

namespace litehtml
{
	struct css_text
	{
		typedef std::vector<css_text>	vector;

		tstring	text;
		tstring	baseurl;
		tstring	media;
		
		css_text() = default;

		css_text(const tchar_t* txt, const tchar_t* url, const tchar_t* media_str)
		{
			text	= txt ? txt : _t("");
			baseurl	= url ? url : _t("");
			media	= media_str ? media_str : _t("");
		}

		css_text(const css_text& val)
		{
			text	= val.text;
			baseurl	= val.baseurl;
			media	= val.media;
		}
	};

	class html_tag;

	class document : public std::enable_shared_from_this<document>
	{
	public:
		typedef std::shared_ptr<document>	ptr;
		typedef std::weak_ptr<document>		weak_ptr;
        std::shared_ptr<element>            m_root;
	private:
		document_container*					m_container;
		fonts_map							m_fonts;
		css_text::vector					m_css;
		litehtml::css						m_styles;
		litehtml::web_color					m_def_color;
		litehtml::context*					m_context;
		litehtml::size						m_size;
		position::vector					m_fixed_boxes;
		media_query_list::vector			m_media_lists;
		element::ptr						m_over_element;
		elements_vector						m_tabular_elements;
		media_features						m_media;
		tstring                             m_lang;
		tstring                             m_culture;
	public:
		document(litehtml::document_container* objContainer, litehtml::context* ctx);
		virtual ~document();

		litehtml::document_container*	container()	{ return m_container; }
		uint_ptr						get_font(const tchar_t* name, int size, const tchar_t* weight, const tchar_t* style, const tchar_t* decoration, font_metrics* fm);
		int								render(int max_width, render_type rt = render_all);
		void							draw(uint_ptr hdc, int x, int y, const position* clip);
		web_color						get_def_color()	{ return m_def_color; }
		int								cvt_units(const tchar_t* str, int fontSize, bool* is_percent = nullptr) const;
		int								cvt_units(css_length& val, int fontSize, int size = 0) const;
		int								width() const;
		int								height() const;
		void							add_stylesheet(const tchar_t* str, const tchar_t* baseurl, const tchar_t* media);
		bool							on_mouse_over(int x, int y, int client_x, int client_y, position::vector& redraw_boxes);
		bool							on_lbutton_down(int x, int y, int client_x, int client_y, position::vector& redraw_boxes);
		bool							on_lbutton_up(int x, int y, int client_x, int client_y, position::vector& redraw_boxes);
		bool							on_mouse_leave(position::vector& redraw_boxes);
		litehtml::element::ptr			create_element(const tchar_t* tag_name, const string_map& attributes);
		element::ptr					root();
		void							get_fixed_boxes(position::vector& fixed_boxes);
		void							add_fixed_box(const position& pos);
		void							add_media_list(const media_query_list::ptr& list);
		bool							media_changed();
		bool							lang_changed();
		bool                            match_lang(const tstring & lang);
		void							add_tabular(const element::ptr& el);
		element::const_ptr		        get_over_element() const { return m_over_element; }

		void                            append_children_from_string(element& parent, const tchar_t* str);
		void                            append_children_from_utf8(element& parent, const char* str);

		static litehtml::document::ptr createFromString(const tchar_t* str, litehtml::document_container* objPainter, litehtml::context* ctx, litehtml::css* user_styles = nullptr);
		static litehtml::document::ptr createFromUTF8(const char* str, litehtml::document_container* objPainter, litehtml::context* ctx, litehtml::css* user_styles = nullptr);

        void init_element(element::ptr element);
        void update_css(litehtml::document::ptr doc, context* ctxt);

        template <typename HtmlTag>
        static litehtml::elements_vector create_child(HtmlTag htmltag, litehtml::document::ptr doc) {
            litehtml::string_map attrs;

            int tagsize = htmltag.name.length();
            litehtml::tchar_t* tag = new char[tagsize];  // do not forget delete it!
            strcpy(tag, htmltag.name.c_str());

            if (!htmltag.properties.empty()) {
                for (auto prop : htmltag.properties) {
                    attrs[prop.first] = prop.second;
                    }
            }

            litehtml::elements_vector elements;
            litehtml::element::ptr ret;
            mtest::structures::HtmlTextOrTags::Variants var = htmltag.tags.currentVariant();

            ret = doc->create_element(tag, attrs);
            if (var == mtest::structures::HtmlTextOrTags::Variants::taglists) {
                for (auto tags : htmltag.tags.tags()) {
                    auto children = litehtml::document::create_child(tags, doc);
                    for (auto child : children) {
                        ret->appendChild(child);
                    }
                }
            } else if (var == mtest::structures::HtmlTextOrTags::Variants::text) {
                std::string t = htmltag.tags.str();
                int sizetext = t.length();
                litehtml::tchar_t* text = new char[sizetext];
                strcpy(text, t.c_str());
                doc->m_container->split_text(
                    text, [doc, &elements](const tchar_t* text) {
                        elements.push_back(std::make_shared<el_text>(text, doc)); 
                    }, 
                    [doc, &elements](const tchar_t* text) { 
                        elements.push_back(std::make_shared<el_space>(text, doc)); 
                    });

                for (auto child : elements) {
                    if (child != nullptr) {
                        ret->appendChild(child);
                    }
                }
            } 

            elements.clear();
            elements.push_back(ret);
            return elements;
        }

		template <typename HtmlTag>
        static litehtml::document::ptr createFromHtmltag(HtmlTag htmltag, litehtml::document_container* objPainter, litehtml::context* ctx, litehtml::css* user_styles) {
            // Create litehtml::document
            litehtml::document::ptr doc = std::make_shared<litehtml::document>(objPainter, ctx);

            // Create litehtml::elements.
            litehtml::elements_vector root_elements;

            litehtml::string_map attrs;
            int tagsize = htmltag.name.length();
            litehtml::tchar_t* tag = new char[tagsize];  
            strcpy(tag, htmltag.name.c_str());

            if (!htmltag.properties.empty()) {
                for (auto prop : htmltag.properties) {
                    attrs[prop.first] = prop.second;
                }
            }

            mtest::structures::HtmlTextOrTags::Variants var = htmltag.tags.currentVariant();
            if (var == mtest::structures::HtmlTextOrTags::Variants::taglists) {
                litehtml::element::ptr ret;

                ret = doc->create_element(tag, attrs);

                if (ret) {
                    root_elements.push_back(ret);
                }
                for (auto tags : htmltag.tags.tags()) {
                    auto children = litehtml::document::create_child(tags, doc);
                    for (auto child : children) {
                        ret->appendChild(child);
                    }
                }
            } else if (var == mtest::structures::HtmlTextOrTags::Variants::text) {
                std::string t = htmltag.tags.str();
                int sizetext = t.length();
                litehtml::tchar_t* text = new char[sizetext];
                strcpy(text, t.c_str());
                root_elements.push_back(std::make_shared<litehtml::el_text>(text, doc));
            }


             if (!root_elements.empty()) {
                doc->m_root = root_elements.back();
             }

            // Let's process created elements tree
            if (doc->m_root) {
                doc->container()->get_media_features(doc->m_media);

                // apply master CSS
                doc->m_root->apply_stylesheet(ctx->master_css());

                // parse elements attributes
                doc->m_root->parse_attributes();

                // parse style sheets linked in document
                litehtml::media_query_list::ptr media;
                for (const auto& css : doc->m_css) {
                    if (!css.media.empty()) {
                        media = litehtml::media_query_list::create_from_string(css.media, doc);
                    } else {
                        media = nullptr;
                    }
                        doc->m_styles.parse_stylesheet(css.text.c_str(), css.baseurl.c_str(), doc, media);
                }
                // Sort css selectors using CSS rules.
                doc->m_styles.sort_selectors();

                // get current media features
                if (!doc->m_media_lists.empty()) {
                    doc->update_media_lists(doc->m_media);
                }

                // Apply parsed styles.
                doc->m_root->apply_stylesheet(doc->m_styles);

                // Apply user styles if any
                if (user_styles) {
                    doc->m_root->apply_stylesheet(*user_styles);
                }

                // Parse applied styles in the elements
                doc->m_root->parse_styles();

                // Now the m_tabular_elements is filled with tabular elements.
                // We have to check the tabular elements for missing table elements
                // and create the anonymous boxes in visual table layout
                doc->fix_tables_layout();

                // Fanaly initialize elements
                doc->m_root->init();
            }

            return doc;
        }

	private:
		litehtml::uint_ptr	add_font(const tchar_t* name, int size, const tchar_t* weight, const tchar_t* style, const tchar_t* decoration, font_metrics* fm);

		void create_node(void* gnode, elements_vector& elements, bool parseTextNode);
		bool update_media_lists(const media_features& features);
		void fix_tables_layout();
		void fix_table_children(element::ptr& el_ptr, style_display disp, const tchar_t* disp_str);
		void fix_table_parent(element::ptr& el_ptr, style_display disp, const tchar_t* disp_str);
	};

	inline element::ptr document::root()
	{
		return m_root;
	}
	inline void document::add_tabular(const element::ptr& el)
	{
		m_tabular_elements.push_back(el);
	}
	inline bool document::match_lang(const tstring & lang)
	{
		return lang == m_lang || lang == m_culture;
	}
}

#endif  // LH_DOCUMENT_H
