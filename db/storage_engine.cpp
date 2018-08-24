// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include "storage_engine.h"

StorageEngine::StorageEngine(Logger* log, int level0_files_number_limit, EventManager* event_manager, StorageBuffer* storage_buffer) 
	: log_(log), 
	  level0_files_number_limit_(level0_files_number_limit),
	  event_manager_(event_manager),
	  storage_buffer_(storage_buffer)
{
	if (access(Constant::DataFolder.c_str(), 0) != 0)
	{
		mkdir(Constant::DataFolder.c_str(), 0777);
	}

	DIR* dir;
	struct dirent* ptr;
	if ((dir = opendir(Constant::DataFolder.c_str())) == NULL)
	{
		log_->Error("Openning DataBase Data Directory Failed.");
	}

	while ((ptr = readdir(dir)) != NULL)
	{
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0
			|| ptr->d_name[0] == '.')	// Prevent some hidden file like ".swp"
		{
			continue;
		}

		std::string file_name(ptr->d_name);
		File* file = new File(file_name);
		level_files_[file->LevelId()].push_back(file);
		files_map_[file->FileId()] = file;
		log_->Info("Read %s File", ptr->d_name);

		// TODO: Give better file id management.
		file_id_ = std::max(file_id_, file->FileId());	
	}

	for (auto& item : level_files_)
	{
		// TODO: Find somewhere else to put the cmp function.
		std::sort(item.second.begin(), item.second.end(), StorageEngine::cmp);
	}

	log_->Info("Reading %d Data Files.", files_map_.size());
}

FILE* StorageEngine::NewWritableFile(int& file_id, std::string& file_name, int level_id)
{
	mutex_.lock();
	file_id = ++file_id_;
	mutex_.unlock();
	file_name = FileName(level_id, file_id);

	// TODO: Duplicate codes, try to reuse the function in "file.h".
	std::string file_path = Constant::DataFolder + std::string("/") + file_name;

	FILE* file_stream = fopen(file_path.c_str(), "w");
	return file_stream;
}

void StorageEngine::AddFile(std::string file_name)
{
	WriteLock();

	File* file = new File(file_name);

	// TODO: Use binary search to optimize this.
	auto it = level_files_[file->LevelId()].begin();
	while (it != level_files_[file->LevelId()].end() && file->LowerBound() > (*it)->LowerBound())
	{
		++it;
	}

	level_files_[file->LevelId()].insert(it, file);
	files_map_[file->FileId()] = file;
	if (level_files_[0].size() > level0_files_number_limit_)
	{
		event_manager_->event_compact_.Notify();
	}

	WriteUnlock();
}

void StorageEngine::GetContainsFiles(std::string& key, std::vector<std::vector<File*>>& contains_files) 
{
	for (auto& item : level_files_)
	{
		std::vector<File*> tmp;
		int i = 0;
		while (i < item.second.size() && key >= item.second[i]->LowerBound())
		{
			if (key <= item.second[i]->UpperBound())
			{
				tmp.push_back(item.second[i]);
			}

			i++;
		}

		std::sort(tmp.begin(), tmp.end(), [](const File* f1, const File* f2) { return f1->FileId() > f2->FileId(); });
		contains_files.push_back(tmp);
	}
}

void StorageEngine::LoadKeyOffset(int file_id, std::unordered_map<std::string, uint32_t>& key_offset)
{
	char* buf = const_cast<char*>(files_map_[file_id]->MMap());
	char* p = buf;
	uint32_t size;
	int length = GetVarint32(p, 5, &size);
	p += length + size;
	length = GetVarint32(p, 5, &size);
	p += length + size;

	uint32_t index_offset;
	GetFixed32(p, &index_offset);

	p = buf + index_offset;
	while (p - buf < files_map_[file_id]->FileSize())
	{
		length = GetVarint32(p, 5, &size);
		p += length;
		std::string key(p, size);
		p += size;
		uint32_t offset;
		length = GetVarint32(p, 5, &offset);
		p += length;
		key_offset[key] = offset;
	}
}

void StorageEngine::GetValueByOffset(int file_id, uint32_t offset, std::string& value_out)
{
	char* p = const_cast<char*>(files_map_[file_id]->MMap());
	p += offset;
	int length = 0;
	uint32_t size;
	length = GetVarint32(p, 5, &size);
	p += length + size;
	length = GetVarint32(p, 5, &size);
	value_out.assign(p + length, size);
}

void StorageEngine::Compact(int level_id)
{
	ReadLock();
	std::map<int, std::vector<File*>> level_files(level_files_);
	ReadUnlock();
	
	if (level_files[level_id].size() <= level0_files_number_limit_ * (int)pow(10, level_id))
	{
		log_->Info("Current Level: %d. This Level File Number: %d. Limit: %d.", level_id, level_files[level_id].size(), level0_files_number_limit_ * (int)pow(10, level_id));
		return;
	}

	int next_level = level_id + 1;

	std::vector<File*> compact_files;	
	compact_files.push_back(level_files[level_id][0]);
	std::string lowerbound = level_files[level_id][0]->LowerBound();
	std::string upperbound = level_files[level_id][0]->UpperBound();

	if (level_id == 0)
	{
		FindOverlapFilesLevel0(level_files[level_id], compact_files, lowerbound, upperbound);
	}
	
	if (level_files.find(next_level) != level_files.end())
	{
		FindOverlapFilesBasedOnBound(level_files[next_level], compact_files, lowerbound, upperbound);
	}


	log_->Info("Starting Level %d Compaction Processing", level_id);
	std::string file_names;
	for (auto& file : compact_files)
	{
		file_names.append("\"" + file->FileName() + "\" ");
	}

	log_->Info("%d Files to be Compacted, including %s", compact_files.size(), file_names.c_str());

	std::vector<File*> compacted_files;

	if (compact_files.size() == 1)
	{
		compacted_files = TrivialMove(compact_files);	
	}
	else
	{
		compacted_files = NWayCompaction(compact_files, level_id);
		UpdateMapAfterCompaction(compacted_files, compact_files, true);
	}

	log_->Info("Ending Level %d Compaction Processing. Compacting %d Old Files, Generating %d New Files.", level_id, compact_files.size(), compacted_files.size());
	file_names = "";
	for (auto& file : compacted_files)
	{
		file_names.append("\"" + file->FileName() + "\" ");
	}

	log_->Info("Generated New Files Includes %s", file_names.c_str());
	Compact(level_id);
	Compact(level_id + 1);
}

std::vector<File*> StorageEngine::TrivialMove(std::vector<File*>& compact_files)
{
	std::vector<File*> compacted_files;
	for (auto& file : compact_files)
	{
		mutex_.lock();
		int file_id = ++file_id_;
		mutex_.unlock();
		
		std::string file_name = FileName(file->LevelId() + 1, file_id);

		// TODO: Duplicate codes, try to reuse the function in "file.h".
		std::string file_path = Constant::DataFolder + std::string("/") + file_name;
		
		rename(file->FilePath().c_str(), file_path.c_str());

		compacted_files.push_back(new File(file_name));
	}

	UpdateMapAfterCompaction(compacted_files, compact_files, false);
	return compacted_files;
}

void StorageEngine::UpdateMapAfterCompaction(std::vector<File*>& compacted_files, std::vector<File*>& compact_files, bool need_remove_file)
{
	WriteLock();

	log_->Info("Starting Updating Level Files Map.");
	for (auto& compact_file : compact_files)
	{
		for (auto it = level_files_[compact_file->LevelId()].begin(); it != level_files_[compact_file->LevelId()].end(); ++it)
		{
			if ((*it)->FileId() == compact_file->FileId())
			{
				log_->Info("Find File %d", compact_file->FileId());
				level_files_[compact_file->LevelId()].erase(it);
				break;
			}
		}

		log_->Info("To Erase file id");
		files_map_.erase(compact_file->FileId());
		log_->Info("Finish Erasing");
	}

	log_->Info("Ending Removing Old Files form Level Files Map.");

	for (auto& compacted_file : compacted_files)
	{
		auto it = level_files_[compacted_file->LevelId()].begin();
		while (it != level_files_[compacted_file->LevelId()].end() && compacted_file->LowerBound() > (*it)->LowerBound())
		{
			++it;
		}

		level_files_[compacted_file->LevelId()].insert(it, compacted_file);
		log_->Info("Inserting %d new file to level files", compacted_file->FileId());
		files_map_[compacted_file->FileId()] = compacted_file;
		log_->Info("Inserting %d new file to files map", compacted_file->FileId());
	}

	log_->Info("Ending Updating Level Files Map.");

	WriteUnlock();

	log_->Info("Starting Releasing Old Files' Resources.");
	for (auto& compact_file : compact_files)
	{
		if (need_remove_file && compact_file->Delete() != 0)
		{
			log_->Info("Delete Old File Failed");
		}

		delete compact_file;
	}

	log_->Info("Ending Releasing Old Files' Resources.");
}

std::vector<File*> StorageEngine::NWayCompaction(std::vector<File*>& compact_files, int level_id)
{
	std::vector<File*> compacted_files;
	int len = compact_files.size();

	std::vector<char*> buf_array(len);
	std::vector<char*> ptr_array(len);
	for (int i = 0; i < len; ++i)
	{
		
		buf_array[i] = const_cast<char*>(compact_files[i]->MMap());
		ptr_array[i] = buf_array[i];

		// Skip the lowerbound and upperbound in the file header
		auto entry = ExtractEntry(ptr_array[i], compact_files[i], i);
		assert(entry.size_ == ptr_array[i] - buf_array[i]);
	}

	std::vector<uint32_t> index_offset(len);
	for (int i = 0; i < len; ++i)
	{
		GetFixed32(ptr_array[i], &index_offset[i]);
		ptr_array[i] += 4;
	}

	std::string prev = "";
	bool has_initial = false;
	std::priority_queue<Entry, std::vector<Entry>, priority_queue_cmp> pq;

	for (int i = 0; i < len; ++i)
	{
		if (ptr_array[i] - buf_array[i] < index_offset[i])
		{
			pq.push(ExtractEntry(ptr_array[i], compact_files[i], i));
		}
	}

	std::vector<ByteArray> content;
	uint32_t content_size = 0;
	while (!pq.empty())
	{
		Entry entry = pq.top();
		pq.pop();
		if (!has_initial || entry.key_ != prev)
		{
			if (!has_initial) 
			{
				has_initial = true;
			}

			prev = entry.key_;

			if (!entry.is_delete_)
			{
				ByteArray temp(entry.p_, entry.size_);

				content.push_back(temp);

				content_size += temp.Size();
				if (content_size > storage_buffer_->BufferSize())
				{
					log_->Info("vector size: %d, content size: %d", content.size(), content_size);
					std::string file_name = FlushCompactedFile(content, content_size, level_id + 1);
					log_->Info("NWay add file %s", file_name.c_str());
					compacted_files.push_back(new File(file_name));
					log_->Info("File id: %d, Level id: %d", compacted_files.back()->FileId(), compacted_files.back()->LevelId());
					content.clear();
					content_size = 0;
				}
			}
		}
		
		if (ptr_array[entry.index_] - buf_array[entry.index_] < index_offset[entry.index_])
		{
			pq.push(ExtractEntry(ptr_array[entry.index_], compact_files[entry.index_], entry.index_));
		}	
	}

	if (!content.empty())
	{
		std::string file_name = FlushCompactedFile(content, content_size, level_id + 1);
		compacted_files.push_back(new File(file_name));
		log_->Info("Final NWay add file %s", file_name.c_str());
		log_->Info("File id: %d, Level id: %d", compacted_files.back()->FileId(), compacted_files.back()->LevelId());
	}

	return compacted_files;
}

std::string StorageEngine::FlushCompactedFile(std::vector<ByteArray>& content, uint32_t content_size, int level_id)
{
	int file_id;
	std::string file_name;
	auto file_stream = NewWritableFile(file_id, file_name, level_id);
	std::unordered_map<std::string, uint32_t> key_offset;

	log_->Info("Starting Flushing Compacted file \"%s\".", file_name.c_str());
	storage_buffer_->Flush(file_stream, content, key_offset, content_size);
	
	log_->Info("Ending Flushing Compacted file \"%s\".", file_name.c_str());
	return file_name;
}

StorageEngine::Entry StorageEngine::ExtractEntry(char*& p, File* file, int index)
{
	uint32_t total_len = 0;
	char* original = p;

	uint32_t size;
	int length = GetVarint32(p, 5, &size);
	p += length;
	std::string key(p, size);
	
	total_len += length + size;

	p += size;
	length = GetVarint32(p, 5, &size);
	p += length;

	total_len += length + size;

	bool is_delete = false;
	if (size == Constant::TombValue.size())
	{
		std::string value(p, size);
		std::string tomb = Constant::TombValue;
		if (value == tomb)
		{
			is_delete = true;
		}
	}

	p += size;
	return Entry(key, file->LevelId(), file->FileId(), is_delete, original, total_len, index);
}

void StorageEngine::FindOverlapFilesBasedOnBound(std::vector<File*>& candidate_files, std::vector<File*>& compact_files, std::string& lowerbound, std::string& upperbound)
{
	for (auto& file : candidate_files)
	{
		if (file->LowerBound() <= upperbound && file->UpperBound() >= lowerbound)
		{
			compact_files.push_back(file);
		}
	}
}

void StorageEngine::FindOverlapFilesLevel0(std::vector<File*>& candidate_files, std::vector<File*>& compact_files, std::string& lowerbound, std::string& upperbound)
{
	for (int i = 1; i < candidate_files.size(); ++i)
	{
		if (candidate_files[i]->LowerBound() <= upperbound)
		{
			compact_files.push_back(candidate_files[i]);
			if (candidate_files[i]->UpperBound() > upperbound)
			{
				upperbound = candidate_files[i]->UpperBound();
			}
		}
		else
		{
			break;
		}
	}
}

