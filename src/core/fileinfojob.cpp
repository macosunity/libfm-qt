#include "fileinfojob.h"
#include "fileinfo_p.h"

namespace Fm {

FileInfoJob::FileInfoJob(FilePathList paths, const std::shared_ptr<const HashSet>& cutFilesHashSet):
    Job(),
    paths_{std::move(paths)},
    cutFilesHashSet_{cutFilesHashSet} {
}

void FileInfoJob::exec() {
    for(const auto& path: paths_) {
        if(isCancelled()) {
            break;
        }
        currentPath_ = path;

        bool retry;
        do {
            retry = false;
            GErrorPtr err;
            GFileInfoPtr inf{
                g_file_query_info(path.gfile().get(), defaultGFileInfoQueryAttribs,
                                  G_FILE_QUERY_INFO_NONE, cancellable().get(), &err),
                false
            };
            if(inf) {
                auto fileInfoPtr = std::make_shared<FileInfo>(inf, path);

                // FIXME: this is not elegant
                if(cutFilesHashSet_
                        && cutFilesHashSet_->count(path.hash())) {
                    fileInfoPtr->bindCutFiles(cutFilesHashSet_);
                }

                results_.push_back(fileInfoPtr);
                Q_EMIT gotInfo(path, results_.back());
            }
            else {
                auto act = emitError(err);
                if(act == Job::ErrorAction::RETRY) {
                    retry = true;
                }
            }
        } while(retry && !isCancelled());
    }
}

} // namespace Fm
