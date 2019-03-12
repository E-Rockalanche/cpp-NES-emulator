#ifndef PROGRAM_END_HPP
#define PROGRAM_END_HPP

// a ProgramEnd object in global scope will perform the callback on program termination

typedef void (*Callback)(void);

class ProgramEnd {
private:
	Callback callback;
public:
	ProgramEnd(Callback callback) { this->callback = callback; }
	~ProgramEnd() { (*callback)(); }
};

#endif