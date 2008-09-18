#include <evoral/Sequence.hpp>
#include <evoral/Transport.hpp>

using namespace Evoral;

class StoppedTransport : public Transport {
	virtual nframes_t transport_frame()   const { return 0; }
	virtual bool      transport_stopped() const { return true; }
};


int
main()
{
	Glib::thread_init();

	StoppedTransport t;
	Sequence s(t, 100);
	return 0;
}
