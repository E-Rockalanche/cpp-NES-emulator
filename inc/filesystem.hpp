#ifndef PATH_HPP
#define PATH_HPP

#include <string>
#include <vector>

namespace fs {

typedef char value_type;
typedef std::basic_string<value_type> string_type;

#ifdef _WIN32
	#define PREFERRED_SEPERATOR '\\'
#else
	#define PREFERRED_SEPERATOR '/'
#endif

class path {
public:
	path() noexcept;
	path(const path& other);
	path(path&& other) noexcept;
	path(const string_type& source);

	~path();

	path& operator=(const path& other);
	path& operator=(path&& other) noexcept;
	path& operator=(string_type&& source);
	path& operator=(const string_type& source);

	path& operator/=(const path& other);
	path& operator/=(const string_type& str);

	path& operator+=(const path& other);
	path& operator+=(const string_type& str);
	path& operator+=(const value_type* cstr);
	path& operator+=(value_type c);

	void clear() noexcept;

	path& make_preferred();

	path& remove_filename();

	path& replace_filename(const path& replacement);

	path& replace_extension(const path& replacement);

	void swap(path& other) noexcept;

	const value_type* c_str() const noexcept;
	const string_type& native() const noexcept;
	operator string_type() const;

	std::string string() const;
	std::wstring wstring() const;
	std::u16string u16string() const;
	std::u32string u32string() const;
	std::string u8string() const;

	std::string generic_string() const;
	std::wstring generic_wstring() const;
	std::u16string generic_u16string() const;
	std::u32string generic_u32string() const;
	std::string generic_u8string() const;

	int compare(const path& other) const noexcept;
	int compare(const string_type& str) const noexcept;
	int compare(const value_type* cstr) const;

	path lexically_normal() const;
	path lexically_relative(const path& base) const;
	path lexically_proximate(const path& base) const;

	path root_name() const;
	path root_directory() const;
	path root_path() const;
	path relative_path() const;
	path parent_path() const;
	path filename() const;
	path stem() const;
	path extension() const;

	bool empty() const noexcept;

	bool has_root_path() const;
	bool has_root_name() const;
	bool has_root_directory() const;
	bool has_relative_path() const;
	bool has_parent_path() const;
	bool has_filename() const;
	bool has_stem() const;
	bool has_extension() const;

	bool is_absolute() const;
	bool is_relative() const;

	friend void swap(path& left, path& right) noexcept;

	friend std::size_t hash_value(const path& p) noexcept;

	friend bool operator==(const path& left, const path& right) noexcept;
	friend bool operator!=(const path& left, const path& right) noexcept;
	friend bool operator<(const path& left, const path& right) noexcept;
	friend bool operator<=(const path& left, const path& right) noexcept;
	friend bool operator>(const path& left, const path& right) noexcept;
	friend bool operator>=(const path& left, const path& right) noexcept;

	friend path operator/(const path& left, const path& right);

private:
	string_type pathname;
};

} //  end namespace fs

#endif