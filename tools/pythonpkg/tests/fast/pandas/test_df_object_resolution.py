import pandas as pd
import duckdb
import datetime
import numpy as np
import pytest

def create_generic_dataframe(data):
    return pd.DataFrame({'0': pd.Series(data=data, dtype='object')})

class TestResolveObjectColumns(object):

    def test_integers(self, duckdb_cursor):
        data = [5, 0, 3]
        df_in = create_generic_dataframe(data)
        # These are float64 because pandas would force these to be float64 even if we set them to int8, int16, int32, int64 respectively
        df_expected_res = pd.DataFrame({'0': pd.Series(data=data)})
        df_out = duckdb.query_df(df_in, "data", "SELECT * FROM data").df()
        print(df_out)
        pd.testing.assert_frame_equal(df_expected_res, df_out)

    def test_correct_struct(self, duckdb_cursor):
        data = [{'a': 1, 'b': 3, 'c': 3, 'd': 7}]
        df = pd.DataFrame({'0': pd.Series(data=data)})
        duckdb_col = duckdb.query("SELECT {a: 1, b: 3, c: 3, d: 7} as '0'").df()
        converted_col = duckdb.query_df(df, "data", "SELECT * FROM data").df()
        pd.testing.assert_frame_equal(duckdb_col, converted_col)

    def test_incorrect_struct_keys(self, duckdb_cursor):
        x = pd.DataFrame(
            [
                [{'a': 1, 'b': 3, 'c': 3, 'd': 7}],
                [{'a': 1, 'b': 3, 'c': 3, 'd': 7}],
                [{'a': 1, 'b': 3, 'c': 3, 'e': 7}], #'e' instead of 'd' as key
                [{'a': 1, 'b': 3, 'c': 3, 'd': 7}],
                [{'a': 1, 'b': 3, 'c': 3, 'd': 7}]
            ]
        )
        with pytest.raises(Exception, match="Struct key on row 2 is incorrect, expected 'd' but encountered 'e'"):
            converted_df = duckdb.query("SELECT * FROM x").df()

    def test_incorrect_struct_key_amount(self, duckdb_cursor):
        x = pd.DataFrame(
            [
                [{'a': 1, 'b': 3, 'c': 3, 'd': 7}],
                [{'a': 1, 'b': 3, 'c': 3, 'd': 7}],
                [{'a': 1, 'b': 3, 'c': 3}],         #incorrect amount of keys
                [{'a': 1, 'b': 3, 'c': 3, 'd': 7}],
                [{'a': 1, 'b': 3, 'c': 3, 'd': 7}]
            ]
        )
        with pytest.raises(Exception, match="Struct entries have differing amounts of fields"):
            converted_df = duckdb.query("SELECT * FROM x").df()
