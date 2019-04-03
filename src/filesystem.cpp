#include <string>
#include <utility>
#include "filesystem.hpp"
#include "debugging.hpp"

#define FS_MAX(a, b) (((a) > (b)) ? (a) : (b))

namespace fs {

path::path() noexcept {}

path::path(const path& other) : pathname(other.pathname) {}

path::path(path&& other) noexcept : pathname(std::move(other.pathname)) {
	other.clear();
}

path::path(const string_type& source) : pathname(source) {}

path::~path() {}

path& path::operator=(const path& other) {
	pathname = other.pathname;
	return *this;
}

path& path::operator=(path&& other) noexcept {
	pathname = std::move(other.pathname);
	return *this;
}

path& path::operator=(string_type&& source) {
	pathname = std::move(source);
	return *this;
}

path& path::operator=(const string_type& source) {
	pathname = source;
	return *this;
}

path& path::operator/=(const path& p) {
	// TODO
	if (p.is_absolute()) {
		*this = p;
	} else {
		pathname += PREFERRED_SEPERATOR;
		pathname += p.pathname;
	}
	return *this;
}

path& path::operator/=(const string_type& str) {
	*this /= path(str);
	return  *this;
}

path& path::operator+=(const path& other) {
	pathname += other.pathname;
	return *this;
}
path& path::operator+=(const string_type& str) {
	pathname += str;
	return *this;
}

path& path::operator+=(const value_type* cstr) {
	pathname += cstr;
	return *this;
}

path& path::operator+=(value_type c) {
	pathname += c;
	return *this;
}

void path::clear() noexcept {
	pathname.clear();
}

path& path::make_preferred() {
	for(auto& c : pathname) {
		if (c == '/' || c == '\\') {
			c = PREFERRED_SEPERATOR;
		}
	}
	return *this;
}

path& path::remove_filename() {
	for(auto it = pathname.rbegin(); it != pathname.rend(); it++) {
		if (*it == '/' || *it == '\\') {
			it--;
			pathname.erase(it.base(), pathname.end());
		}
	}
	return *this;
}

path& path::replace_filename(const path& replacement) {
	remove_filename();
	pathname += replacement.pathname;
	return *this;
}

path& path::replace_extension(const path& replacement = path()) {
	for(auto it = pathname.rbegin(); it != pathname.rend(); it++) {
		if (*it == '.') {
			if (!replacement.empty() && replacement.pathname[0] == '.') it++;
			pathname.erase(it.base(), pathname.end());
		}
	}
	pathname += replacement.pathname;
	return *this;
}

void path::swap(path& other) noexcept {
	string_type temp = pathname;
	pathname = other.pathname;
	other.pathname = pathname;
}

const value_type* path::c_str() const noexcept {
	return pathname.c_str();
}

const string_type& path::native() const noexcept {
	return pathname;
}

path::operator string_type() const {
	return pathname;
}

std::string path::string() const {
	return pathname;
}

std::string path::generic_string() const {
	return pathname;
}

int path::compare(const path& other) const noexcept {
	int result = root_name().native().compare(other.root_name().native());
	if (result != 0) {
		return result;
	} else if (has_root_directory() != other.has_root_directory()) {
		return has_root_directory() ? 1 : -1;
	} else {
		auto my_it = pathname.begin();
		auto other_it = other.pathname.begin();
		while(my_it != pathname.end() && other_it != other.pathname.end()) {
			result = *my_it - *other_it;
			if (result != 0) return result;
			my_it++;
			other_it++;
		}
		if (other_it != other.pathname.end()) return 1;
		else if (my_it != pathname.end()) return -1;
	}
	return 0;
}
int path::compare(const string_type& str) const noexcept {
	return compare(path(str));
}
int path::compare(const value_type* cstr) const {
	return compare(path(cstr));
}

path path::lexically_normal() const {
	string_type directory;
	string_type filename;
	if (pathname.empty()) return path();
	else {
		bool last_was_slash = false;
		for(auto c : pathname) {
			bool is_slash = c == '/' || c == '\\';
			if (is_slash) {
				if (!last_was_slash) {
					if (filename != ".") {
						directory += filename;
						directory += PREFERRED_SEPERATOR;
					} else if (filename == "..") {
						for(auto it = directory.rbegin(); it != directory.rend(); it++) {
							if ((*it) == PREFERRED_SEPERATOR) {
								directory.erase(it.base(), directory.end());
							}
						}
					}
				}
			} else {
				filename += c;
			}
			last_was_slash = is_slash;
		}
	}
	return path(directory + filename);
}

path path::root_name() const {
	// TODO
	return path();
}

path path::root_directory() const {
	// TODO
	return path();
}

path path::parent_path() const {
	int end;
	for(end = pathname.size()-1; end >= 0; end--) {
		char c = pathname[end];
		if (c == '/' || c == '\\') {
			return path(string_type(pathname, 0, FS_MAX(end, 1)));
		}
	}
	return path();
}

path path::filename() const {
	int start;
	for(start = pathname.size()-1; start >= 0; start--) {
		char c = pathname[start];
		if (c == '/' || c == '\\') {
			start++;
			break;
		}
	}
	return path(string_type(pathname, start));
}

path path::stem() const {
	int start;
	int end = pathname.size();
	bool found_ext = false;
	for(start = pathname.size()-1; start >= 0; start--) {
		char c = pathname[start];
		if (c == '.' && !found_ext) {
			end = start;
			found_ext = true;
		} else if (c == '/' || c == '\\') {
			start++;
			break;
		}
	}
	return path(string_type(pathname, start, end-start));
}

path path::extension() const {
	int start;
	for(start = pathname.size()-1; start >= 0; start--) {
		if (pathname[start] == '.') {
			return path(string_type(pathname, start+1));
		}
	}
	return path();
}

bool path::empty() const noexcept {
	return pathname.empty();
}

bool path::has_root_name() const {
	// TODO
	return false;
}

bool path::has_root_directory() const {
	// TODO
	return false;
}

bool path::has_parent_path() const {
	return !parent_path().empty();
}

bool path::has_filename() const {
	return !filename().empty();
}

bool path::has_stem() const {
	return !stem().empty();
}

bool path::has_extension() const {
	return !extension().empty();
}

bool path::is_absolute() const {
	return !is_relative();
}

bool path::is_relative() const {
	return pathname.empty() || (pathname[0] != '/' && pathname[0] != '\\');
}

void swap(path& left, path& right) noexcept {
	string_type temp = left.pathname;
	left.pathname = right.pathname;
	right.pathname = temp;
}

std::size_t hash_value(const path& p) noexcept {
    std::hash<std::string> hash_fn;
    return hash_fn(p.pathname);
}

bool operator==(const path& left, const path& right) noexcept {
	return !(left > right) && !(left < right);
}

bool operator!=(const path& left, const path& right) noexcept {
	return !(left == right);
}

bool operator<(const path& left, const path& right) noexcept {
	return left.compare(right) < 0;
}

bool operator<=(const path& left, const path& right) noexcept {
	return !(left > right);
}

bool operator>(const path& left, const path& right) noexcept {
	return left.compare(right) > 0;
}

bool operator>=(const path& left, const path& right) noexcept {
	return !(left < right);
}

path operator/(const path& left, const path& right) {
	path temp(left);
	temp /= right;
	return temp;
}

} // end namespace fs