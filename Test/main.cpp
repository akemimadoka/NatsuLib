#include <iostream>
#include <sstream>

#include <natUtil.h>
#include <natMisc.h>
#include <natConcepts.h>
#include <natLog.h>
#include <natMultiThread.h>
#include <natLinq.h>
#include <natStackWalker.h>
#include <natString.h>
#include <natStream.h>
#include <natStreamHelper.h>
#include <natVFS.h>
#include <natLocalFileScheme.h>
#include <natCompression.h>
#include <natCompressionStream.h>
#include <natRelationalOperator.h>
#include <natProperty.h>

using namespace NatsuLib;

struct Incrementable
{
	template <typename T>
	auto requires_(T&& x) -> decltype(++x);

	int foo;
};

template <typename T>
REQUIRES(void, std::conjunction<Models<Incrementable(T)>>::value) increment(T& x)
{
	++x;
}

HasMemberTrait(foo);

int main()
{
	natEventBus eventBus;
	natLog logger(eventBus);
	natConsole console;
	logger.UseDefaultAction(console);

#ifdef _WIN32
	console.WriteLine(console.GetTitle());
	console.SetTitle("NatsuLib test"_nv);
	console.SetColor(natConsole::ConsoleColorTarget::Foreground, natConsole::ConsoleColor::Green);
	//console.SetColor(natConsole::ConsoleColorTarget::Background, natConsole::ConsoleColor::Magenta);
#endif

	try
	{
#ifdef _WIN32
		logger.LogMsg("{1} {0}"_nv, u8"≤‚ ‘÷–Œƒ"_nv, 123);
		std::cout << "baka"_nv << std::endl;
		std::wcout << "baka"_nv << std::endl;
		logger.LogMsg("%s", "baka"_nv);
#endif

		constexpr auto test = HasMemberNamedfoo<Incrementable>::value;
		static_cast<void>(test);

		int t = 5;
		increment(t);
		auto scope = make_scope([&logger](int i)
		{
			logger.LogMsg("%s%d"_nv, "end"_nv, i);
		}, t);
		
		{
			"test 2333"_nv.Split(" 2"_nv, [&logger](nStrView const& str)
			{
				logger.LogMsg(str);
			});
		}

		/*{
			logger.LogMsg("Input: "_nv);
			logger.LogMsg("Your input: {0}"_nv, console.ReadLine());
		}*/

		{
			logger.LogMsg(Environment::GetEnvironmentVar("PATH"_nv));
		}
		
		{
			int arr[] = { 1, 2, 3, 4, 5 };
			for (auto&& item : from(arr).select([](int i){ return i + 1; }).where([](int i){ return i > 3; }))
			{
				logger.LogMsg("%d"_nv, item);
			}
			logger.LogMsg("%d"_nv, from_values(2, 4, 6, 3, 2).where([](int i) { return i >= 4; }).count());
		}

		{
			natThreadPool pool{ 2, 4 };
			auto ret = pool.QueueWork([](void* Param)
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(10ms);
				static_cast<natLog*>(Param)->LogMsg("test"_nv);
				return 0u;
			}, &logger);
			auto&& result = ret.get();
			logger.LogMsg("Work started at thread index {0}, id {1}."_nv, result.GetWorkThreadIndex(), pool.GetThreadId(result.GetWorkThreadIndex()));
			logger.LogMsg("Work finished with result {0}."_nv, result.GetResult().get());
			pool.WaitAllJobsFinish();
		}
#ifdef EnableStackWalker
		{
			natStackWalker stackWalker;
			stackWalker.CaptureStack();
			logger.LogMsg("Call stack:"_nv);
			for (size_t i = 0; i < stackWalker.GetFrameCount(); ++i)
			{
				auto&& symbol = stackWalker.GetSymbol(i);
#ifdef _WIN32
				logger.LogMsg("{3}: (0x%p) {4} at address 0x%p (file {5}:{6} at address 0x%p)"_nv, symbol.OriginalAddress, reinterpret_cast<const void*>(symbol.SymbolAddress), reinterpret_cast<const void*>(symbol.SourceFileAddress), i, symbol.SymbolName, symbol.SourceFileName, symbol.SourceFileLine);
#else
				logger.LogMsg("0x%p : {1}"_nv, symbol.OriginalAddress, symbol.SymbolInfo);
#endif
			}
		}
#endif
		{
			natCriticalSection cs;
			natRefScopeGuard<natCriticalSection> sg(cs);
		}

		{
			natVFS vfs;
			auto req = static_cast<natRefPointer<LocalFileRequest>>(vfs.CreateRequest("file:///main.cpp"));
#ifdef _WIN32
			req->SetAsync(true);
#endif
			auto fs = req->GetResponse()->GetResponseStream();
			std::vector<nByte> buffer(static_cast<size_t>(fs->GetSize()));
			logger.LogMsg("Read {0} bytes."_nv, fs->ReadBytesAsync(buffer.data(), buffer.size()).get());
#ifdef _WIN32
			buffer.push_back(0);
			logger.LogMsg(nString{ AnsiStringView{ reinterpret_cast<const char*>(buffer.data()) } });
#endif
		}

		{
			Uri a{ "http://test:2333@funamiyui.moe:80/blog/index.html?index=5#main"_nv };
			logger.LogMsg("{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}"_nv, a.GetScheme(), a.GetUser(), a.GetPassword(), a.GetHost(), a.GetPort().value_or(80u), a.GetPath(), a.GetQuery(), a.GetFragment());
		}

		{
			struct RefTest : natRefObjImpl<RefTest, natRefObj>
			{
				RefTest()
				{
					std::cout << "constructed" << std::endl;
				}

				~RefTest()
				{
					std::cout << "destroyed" << std::endl;
				}
			};

			auto pTest = make_ref<RefTest>();
			const auto pWeakTest = pTest->ForkWeakRef<RefTest>();
			pTest = nullptr;
			logger.LogMsg("%b"_nv, pWeakTest.IsExpired());
		}

		{
			nByte buffer[128]{};
			size_t dataLength;
			{
				natDeflateStream str{ make_ref<natExternMemoryStream>(buffer, 128, true, true), natDeflateStream::CompressionLevel::Optimal };
				dataLength = str.WriteBytes(reinterpret_cast<ncData>("3"), 1);
			}
			{
				natDeflateStream instr{ make_ref<natExternMemoryStream>(buffer, dataLength, true, true) };
				nByte inflatedData[128]{};
				instr.ReadBytes(inflatedData, sizeof inflatedData);
				assert(memcmp(inflatedData, "3", 1) == 0);
			}
		}

		{
			{
				natZipArchive zip{ make_ref<natFileStream>("1.zip"_nv, true, false), natZipArchive::ZipArchiveMode::Read };
				const auto entry = zip.GetEntry("1/2.txt"_nv);
				nByte data[128]{};
				const auto stream = entry->Open();
				stream->ReadBytes(data, sizeof data);
				stream.GetRefCount();
			}
			{
#ifdef _WIN32
				auto fileStream = make_ref<natFileStream>("2.zip"_nv, true, true);
				fileStream->SetSize(0);
#else
				auto fileStream = make_ref<natFileStream>("2.zip"_nv, true, true, true);
#endif
				{
					natZipArchive zip{ fileStream, natZipArchive::ZipArchiveMode::Create };
					const auto entry = zip.CreateEntry("1.txt"_nv);
					entry->SetPassword("2333"_nv);
					const auto stream = entry->Open();
					stream->WriteBytes(reinterpret_cast<ncData>("2333"), 4);
				}
				fileStream->SetPosition(NatSeek::Beg, 0);
				{
					natZipArchive zip{ fileStream, natZipArchive::ZipArchiveMode::Read };
					const auto entry = zip.GetEntry("1.txt"_nv);
					entry->SetPassword("2333"_nv);
					const auto stream = entry->Open();
					switch (entry->GetDecryptStatus())
					{
					case natZipArchive::ZipEntry::DecryptStatus::Success:
						logger.LogMsg("Decrypt succeed."_nv);
						break;
					case natZipArchive::ZipEntry::DecryptStatus::Crc32CheckFailed:
						logger.LogWarn("Decrypt failed: Crc32 check failed, password may be unmatched."_nv);
						break;
					case natZipArchive::ZipEntry::DecryptStatus::NotDecryptYet:
						logger.LogErr("Decrypt failed: Internal error."_nv);
						break;
					case natZipArchive::ZipEntry::DecryptStatus::NeedNotToDecrypt:
						logger.LogMsg("Need not to decrypt."_nv);
						break;
					default:
						assert(!"Invalid DecryptStatus");
						break;
					}
					nByte buffer[128]{};
					stream->ReadBytes(buffer, 128);
					assert(memcmp(buffer, "2333", 4) == 0);
				}
				fileStream->SetPosition(NatSeek::Beg, 0);
				{
					natZipArchive zip{ fileStream, natZipArchive::ZipArchiveMode::Update };
					zip.CreateEntry("test/"_nv);
					const auto entry = zip.CreateEntry("test/2.txt"_nv);
					const auto stream = entry->Open();
					stream->WriteBytes(reinterpret_cast<ncData>("1234"), 4);
					//zip.GetEntry("1.txt"_nv)->Delete();
				}
			}
		}

		{
			struct Integer
				: natRefObjImpl<Integer, RelationalOperator::IComparable<int>>
			{
				nInt CompareTo(int const& other) const override
				{
					return m_Value - other;
				}

			private:
				int m_Value{};

			public:
				Property<int> value{ m_Value, AutoPropertyFlags::All };
			};

			Integer a;
			a.value = 5;
			logger.LogMsg("a < 3:%b"_nv, a < 3);
			logger.LogMsg("7 > a:%b"_nv, 7 > a);
			logger.LogMsg("-3 <= a:%b"_nv, -3 <= a);
			logger.LogMsg("5 >= a:%b"_nv, 5 >= a);
		}
	}
#ifdef _WIN32
	catch (natWinException& e)
	{
		logger.LogErr("Exception caught from {0}, file \"{1}\" line {2},\nDescription: {3}\nErrno: {4}, Msg: {5}"_nv, e.GetSource(), e.GetFile(), e.GetLine(), e.GetDesc(), e.GetErrNo(), e.GetErrMsg());
#ifdef EnableExceptionStackTrace
		logger.LogErr("Call stack:"_nv);
		for (size_t i = 0; i < e.GetStackWalker().GetFrameCount(); ++i)
		{
			auto&& symbol = e.GetStackWalker().GetSymbol(i);
			logger.LogErr("{3}: (0x%p) {4} at address 0x%p (file {5}:{6} at address 0x%p)"_nv, symbol.OriginalAddress, reinterpret_cast<const void*>(symbol.SymbolAddress), reinterpret_cast<const void*>(symbol.SourceFileAddress), i, symbol.SymbolName, symbol.SourceFileName, symbol.SourceFileLine);
		}
#endif
	}
#endif
	catch (natErrException& e)
	{
		logger.LogErr("Exception caught from {0}, file \"{1}\" line {2},\nDescription: {3}\nErrno: {4}, Msg: {5}"_nv, e.GetSource(), e.GetFile(), e.GetLine(), e.GetDesc(), e.GetErrNo(), e.GetErrMsg());
#ifdef EnableExceptionStackTrace
		logger.LogErr("Call stack:"_nv);
		for (size_t i = 0; i < e.GetStackWalker().GetFrameCount(); ++i)
		{
			auto&& symbol = e.GetStackWalker().GetSymbol(i);
#ifdef _WIN32
			logger.LogErr("{3}: (0x%p) {4} at address 0x%p (file {5}:{6} at address 0x%p)"_nv, symbol.OriginalAddress, reinterpret_cast<const void*>(symbol.SymbolAddress), reinterpret_cast<const void*>(symbol.SourceFileAddress), i, symbol.SymbolName, symbol.SourceFileName, symbol.SourceFileLine);
#else
			logger.LogErr("0x%p : {1}"_nv, symbol.OriginalAddress, symbol.SymbolInfo);
#endif
		}
#endif
	}
	catch (natException& e)
	{
		logger.LogErr("Exception caught from {0}, file \"{1}\" line {2},\nDescription: {3}"_nv, e.GetSource(), e.GetFile(), e.GetLine(), e.GetDesc());
#ifdef EnableExceptionStackTrace
		logger.LogErr("Call stack:"_nv);
		for (size_t i = 0; i < e.GetStackWalker().GetFrameCount(); ++i)
		{
			auto&& symbol = e.GetStackWalker().GetSymbol(i);
#ifdef _WIN32
			logger.LogErr("{3}: (0x%p) {4} at address 0x%p (file {5}:{6} at address 0x%p)"_nv, symbol.OriginalAddress, reinterpret_cast<const void*>(symbol.SymbolAddress), reinterpret_cast<const void*>(symbol.SourceFileAddress), i, symbol.SymbolName, symbol.SourceFileName, symbol.SourceFileLine);
#else
			logger.LogErr("0x%p : {1}"_nv, symbol.OriginalAddress, symbol.SymbolInfo);
#endif
		}
#endif
	}

#ifdef _WIN32
	system("pause");
#else
	getchar();
#endif
}
