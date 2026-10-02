#include <xml/xml.hxx>

xml::attribute::attribute()
    : val()
{
}

xml::attribute::attribute(const value_type &value)
    : val(value)
{
}

xml::attribute &xml::attribute::operator=(const value_type &value)
{
    val = value;
    return *this;
}

xml::attribute::attribute(value_type &&value)
    : val(std::move(value))
{
}

xml::attribute &xml::attribute::operator=(value_type &&value)
{
    val = std::move(value);
    return *this;
}

std::string xml::element::get_text() const
{
    std::string text;
    for (const auto &node : nodes)
    {
        if (node.is<element>())
            text += node.get<element>().get_text();
        else if (node.is<string>())
            text += node.get<string>();
    }
    return text;
}

const xml::element *xml::element::find(const std::string &key) const
{
    const auto it = elements_map.find(key);
    if (it == elements_map.end())
        return nullptr;

    const auto &vec = it->second;
    if (vec.empty())
        return nullptr;

    return vec.front();
}

std::vector<const xml::element *> xml::element::find_all(const std::string &key) const
{
    const auto it = elements_map.find(key);
    if (it == elements_map.end())
        return {};

    return it->second;
}
