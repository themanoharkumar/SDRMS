#ifndef GRAPH_WIDGET_H
#define GRAPH_WIDGET_H

#include <QWidget>
#include <vector>

class ApplicationModel;

class RoadGraphWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RoadGraphWidget(ApplicationModel *model, QWidget *parent = nullptr);

    void setHighlightedPath(const std::vector<int> &path);
    void clearHighlight();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    ApplicationModel *model_;
    std::vector<int> highlightedPath_;
};

#endif // GRAPH_WIDGET_H
