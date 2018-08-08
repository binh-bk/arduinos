from flask import Flask, render_template

import json
import plotly
from plotly import tools
import plotly.graph_objs as go
import sqlite3

app = Flask(__name__)



class SqlReader:

    def getData(self):
        #connection = sqlite3.connect('/fullpath/to/sqlit3/pirs.db')
        connection = sqlite3.connect('pirs.db')
        cursor = connection.cursor()

        sql_command = """
        SELECT * from  pirone
        ORDER BY thetime DESC
        LIMIT 2000;"""

        cursor.execute(sql_command)
        result = cursor.fetchall()

        cursor.close()
        return result


@app.route('/')
def index():
    sql = SqlReader()
    result = sql.getData()

    numOfPoints = len(result)

    pir = [yValues[1] for yValues in result]
    xValues = [xValues[0] for xValues in result]

    graphs = [
       
       dict(
            data=[
                dict(
                    x=xValues,
                    y=pir,
                    type='bar'
                ),
            ],
            layout=dict(
                title='Motion vs Time'
            )
        )
    ]

    # Add "ids" to each of the graphs to pass up to the client
    # for templating
    ids = ['Graph {}'.format(i) for i, _ in enumerate(graphs)]

    # Convert the figures to JSON
    # PlotlyJSONEncoder appropriately converts pandas, datetime, etc
    # objects to their JSON equivalents
    graphJSON = json.dumps(graphs, cls=plotly.utils.PlotlyJSONEncoder)

    return render_template('layouts/index.html',
                           ids=ids,
                           graphJSON=graphJSON)


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=9999)
