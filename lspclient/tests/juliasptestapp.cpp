#include "../lspclientserver.h"
#include <iostream>

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QJsonObject>
#include <QTextStream>
#include <QDebug>

/*
 * the program requires at least 4 arguments:
 *
 * 1. Server command (can contain spaces)
 * 2. Server URL
 * 3. Document URL (path to an existing file)
 * 4. Position in document as "row column"
 * For example:
 * ./build/bin/lsptestapp "clangd --background-index" "file:///tmp/server" "file:////path/to/your/cpp/file.cpp" "0 0"
 * Concrete example:
 * ./bin/lsptestapp "clangd --background-index --log=verbose" "stdio://" "file://${HOME}/Projects/kdev-julia/lspclient/tests/lsptestapp.cpp" "10 15"
 * You should replace:
 * - `clangd --background-index` with your desired language server
 * - `file:///tmp/server` with the server URL
 * - `/path/to/your/cpp/file.cpp` with a path to an existing C++ file
 * - `10 15` with line and column numbers within that file
 */

int main(int argc, char **argv)
{
    //  julia server constants
    QString julia = QStringLiteral("${HOME}/.juliaup/bin/julialauncher");
    QString juliaServerEnv = QStringLiteral("--project=${HOME}/.julia/environments/v1.11/Project.toml");
    QString serverEnv = QStringLiteral("${HOME}/.julia/environments/v1.11/Project.toml");

    QStringList command = { julia
        , juliaServerEnv
        , QStringLiteral( "-e" )
        , QStringLiteral( "using LanguageServer; runserver();" )
        /*,QStringLiteral( "" )*/ };
    QString serverURL = QStringLiteral("stdio://");

    QString path = QStringLiteral( "file://${HOME}/.julia/packages/LanguageServer/Fwm1f/src/document.jl" );
    QString pos = QStringLiteral ( "0 0" );


    QCoreApplication app(argc, argv);

    LSPClientServer lsp( command, QUrl(serverURL) );

    QEventLoop q;

    auto state_h = [&lsp, &q]() {
        if (lsp.state() == LSPClientServer::State::Running) {
            q.quit();
        }
    };
    auto conn = QObject::connect(&lsp, &LSPClientServer::stateChanged, state_h);
    lsp.start(true);
    q.exec();
    QObject::disconnect(conn);

    auto diagnostics_h = [](const LSPPublishDiagnosticsParams &diag) {
        std::cout << "diagnostics  " << diag.uri.toLocalFile().toUtf8().toStdString() << " count: " << diag.diagnostics.length();
    };

    QObject::connect(&lsp, &LSPClientServer::publishDiagnostics, diagnostics_h);

    auto document = QUrl( path );

    QFile file(document.toLocalFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return -1;
    }
    QTextStream in(&file);
    QString content = in.readAll();
    lsp.didOpen(document, 0, QString(), content);

    auto ds_h = [&q](const std::list<LSPSymbolInformation> &syms) {
        std::cout << "symbol count: " << syms.size() << std::endl;
        q.quit();
    };
    lsp.documentSymbols(document, &app, ds_h);
    q.exec();

    auto position = pos.split(QLatin1Char(' '));
    auto def_h = [&q](const QList<LSPLocation> &defs) {
        std::cout << "definition count: " << defs.length() << std::endl;
        q.quit();
    };
    lsp.documentDefinition(document, {position[0].toInt(), position[1].toInt()}, &app, def_h);
    q.exec();

    auto comp_h = [&q](const QList<LSPCompletionItem> &completions) {
        std::cout << "completion count: " << completions.length() << std::endl;
        q.quit();
    };
    lsp.documentCompletion(document, {position[0].toInt(), position[1].toInt()}, &app, comp_h);
    q.exec();

    auto sig_h = [&q](const LSPSignatureHelp &help) {
        std::cout << "signature help count: " << help.signatures.length() << std::endl;
        q.quit();
    };
    lsp.signatureHelp(document, {position[0].toInt(), position[1].toInt()}, &app, sig_h);
    q.exec();

    auto hover_h = [&q](const LSPHover &hover) {
        for (auto &element : hover.contents) {
            std::cout << "hover: " << element.value.toStdString() << std::endl;
        }
        q.quit();
    };
    lsp.documentHover(document, {position[0].toInt(), position[1].toInt()}, &app, hover_h);
    q.exec();

    auto ref_h = [&q](const QList<LSPLocation> &refs) {
        std::cout << "refs: " << refs.length() << std::endl;
        q.quit();
    };
    lsp.documentReferences(document, {position[0].toInt(), position[1].toInt()}, true, &app, ref_h);
    q.exec();

    auto hl_h = [&q](const QList<LSPDocumentHighlight> &hls) {
        std::cout << "highlights: " << hls.length() << std::endl;
        q.quit();
    };
    lsp.documentHighlight(document, {position[0].toInt(), position[1].toInt()}, &app, hl_h);
    q.exec();

    auto fmt_h = [&q](const QList<LSPTextEdit> &edits) {
        std::cout << "edits: " << edits.length() << std::endl;
        q.quit();
    };
    lsp.documentFormatting(document, {.tabSize = 2, .insertSpaces = true, .extra = QJsonObject()}, &app, fmt_h);
    q.exec();

    // lsp.didOpen(document, 0, QStringLiteral("blah"));
    lsp.didChange(document, 1, QStringLiteral("foo"));
    lsp.didClose(document);
}

