#include <gtkmm.h>
#include "sequence.h"
#include "seqedit.h"

using namespace Gtk;

int
main(int argc, char *argv[])
{
	Gtk::Main kit(argc, argv);

	//   Gtk::Window window;

	/* scroll bars */
	Adjustment *m_vadjust = manage(new Adjustment(55, 0, c_num_keys, 1, 1, 1));
	Adjustment *m_hadjust = manage(new Adjustment(0, 0, 1, 1, 1, 1));
	VScrollbar *m_vscroll_new = manage(new VScrollbar(*m_vadjust));
	HScrollbar *m_hscroll_new = manage(new HScrollbar(*m_hadjust));

	int m_zoom = 1;
	int m_snap = 1;
	int m_pos = 0;

	sequence *seq = new sequence();
	seqedit *editor = new seqedit(seq, 0);

	Gtk::Main::run(*editor);

	return 0;
}
