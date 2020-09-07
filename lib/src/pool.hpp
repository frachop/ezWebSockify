//
//  pool.hpp
//  ezWebSockify
//
//  Created by Franck  on 22/07/2020.
//  Copyright © 2020 Frachop. All rights reserved.
//

#pragma once

namespace ezWebSockify {

	// - /////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct IStream
	{
		virtual Frame* get() = 0;
		virtual bool empty() const = 0;
		virtual void release( Frame * f ) = 0;
	};

	struct OStream
	{
		virtual Frame* reserve() = 0;
		virtual void add(Frame* f) = 0;
	};

	// - /////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	class FramePool
	{
	public:
		Frame * get() {
			static int count{0};
			std::unique_lock<std::mutex> l{ _m };
			
			LOGT("get frame from pool...");
 			if (_pool.empty())
			{
				count++;
				LOGT("... need to create new frame [ {} ]", count);
			}
				
			Frame * res= (_pool.empty()) ? new Frame : _pool.front();
			if (! _pool.empty())
				_pool.pop_front();
			
			LOGT("...pool count {}", _pool.size());
			res->setStatus( Frame::Status::readyToRead );
			return res;
		}
		
		void release(Frame *f)
		{
			assert(f);
			std::unique_lock<std::mutex> l{ _m };
			assert( std::find( std::begin(_pool), std::end(_pool), f) == std::end(_pool));
			_pool.push_back(f);
			f->setStatus( Frame::Status::released );
			LOGT("releasing frame [ {} ]", _pool.size());
			//Frame::dumpAll();

		}
		
	private:
		std::mutex _m{};
		std::list<Frame*> _pool{};
	};
	
	// - /////////////////////////////////////////////////////////////////////////////////////////////////////////

	class IOStream
		:	public IStream
		,	public OStream
	{
	public:
		explicit IOStream(FramePool & pool)
		:	IStream{}
		,	OStream{}
		,	_m{}
		,	_pool{pool}
		{}
	
	
		virtual Frame* reserve() override
		{
			LOGTFN;
			return _pool.get();
		}
	
		virtual void add(Frame* f) override
		{
			LOGTFN;
			
			std::unique_lock<std::mutex> l{ _m };
			assert( f );
			assert( std::find(std::begin(_inStream), std::end(_inStream), f ) == std::end( _inStream ) );
			_inStream.push_back(f);
			f->setStatus( Frame::Status::readed );
			LOGT("adding frame to inStream[ {} ]", _inStream.size());

		}

		virtual bool empty() const override
		{
			std::unique_lock<std::mutex> l{ _m };
			return _inStream.empty();
		}

		virtual Frame* get() override
		{
			//LOGTFN;
			
			std::unique_lock<std::mutex> l{ _m };
			if (_inStream.empty())
				return nullptr;

			Frame* res = _inStream.front();
			_inStream.pop_front();
			LOGT("getting frame from inStream[ {} ]", _inStream.size());
			res->setStatus( Frame::Status::waitToWrite );
			return res;
		}

		virtual void release(Frame* f) override
		{
			LOGTFN;
			_pool.release(f);
		}

	private:
		mutable std::mutex _m;
		FramePool & _pool;
		std::list< Frame* > _inStream;
	};

	class Bridge
	{
		IStream & _in ;
		OStream & _out;
		
	public:
		Bridge( IStream & in, OStream & out)
		:	_in(in)
		,	_out(out)
		{}
		virtual ~Bridge() {}
		
		IStream & in () { return _in; }
		OStream & out() { return _out; }
	};


}
