===================
artdaq Design Notes
===================

Data flow
=========

Definitions
-----------

A 'Fragment' contains a vector of data words.

A 'FramentHandle' provides unique ownership of a
Fragment. FragmentPtrs can't be copied, but can be moved.

A 'data word' is an unsigned integer of a size determined to be
natural for the problem being solved; this will usually be the natural
size of an integer on the DAQ platform. Currently, this is a 64-bit
unsigned integer.

Flow
----

The SHandles and RHandles classes manage special memory buffers for
sending and receiving data. The data must be copied from these
buffers; we can not give the memory of these buffers to be owned by
the art::Event. The copy should be made to memory that *can* be given
over to the art::Event. This means using unique_ptr<Fragment> as the
"handle" used to control the Fragment; unique_ptr<Fragment> is the
type named as FragmentPtr.

The DS50EventReader is responsible for creating Fragments, housed in a
vector of FragmentPtrs. It can read data files (not yet implemented,
at the time of this writing) and generate random data (also not yet
implemented).
